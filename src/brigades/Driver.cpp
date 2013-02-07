#include <string.h>
#include <algorithm>

#include "Driver.h"
#include "SensorySystem.h"

#include "common/Random.h"
#include "common/Rectangle.h"
#include "common/SDL_utils.h"

using namespace Common;

namespace Brigades {

static int screenWidth = 800;
static int screenHeight = 600;

bool Sprite::operator<(const Sprite& s1) const
{
	/* Important to define strict weak ordering or face segmentation fault. */
	auto e1 = mPosition;
	auto e2 = s1.mPosition;
	if(e1.z < e2.z)
		return true;
	if(e1.z > e2.z)
		return false;
	if(e1.y > e2.y)
		return true;
	return false;
}

bool SpeechBubble::expired(float t)
{
	mTime.doCountdown(t);
	return mTime.check();
}

Driver::Driver(WorldPtr w, bool observer, SoldierRank r)
	: mWorld(w),
	mPaused(false),
	mScaleLevel(11.5f),
	mScaleLevelVelocity(0.0f),
	mFreeCamera(false),
	mSoldierVisible(false),
	mObserver(observer),
	mShooting(false),
	mRestarting(false),
	mDriving(false),
	mSpeechBubbleTimer(2.0f),
	mNextInfoMessageIndex(0),
	mSoldierRank(r),
	mCreatingRectangle(false),
	mRectangleFinished(false),
	mChangeFocus(false),
	mTimeAcceleration(1.0f),
	mDigging(false)
{
	mScreen = SDL_utils::initSDL(screenWidth, screenHeight, "Brigades");

	loadTextures();
	loadFont();
	SDL_utils::setupOrthoScreen(screenWidth, screenHeight);
}

void Driver::init()
{
	setFocusSoldier();
}

void Driver::run()
{
	double prevTime = Clock::getTime();
	while(1) {
		double newTime = Clock::getTime();
		double frameTime = newTime - prevTime;
		prevTime = newTime;
		frameTime = std::min(frameTime, 0.1);

		if(!mPaused) {
			float ta = mTimeAcceleration;
			while(ta >= 1.0f) {
				mWorld->update(frameTime);
				ta--;
			}
			if(ta > 0.0f) {
				mWorld->update(ta * frameTime);
			}
		}

		{
			mSpeechBubbleTimer.doCountdown(frameTime);
			if(mSpeechBubbleTimer.check()) {
				mSpeechBubbleTimer.rewind();
				for(auto it = mSpeechBubbles.begin(); it != mSpeechBubbles.end(); ) {
					if(it->second.expired(mSpeechBubbleTimer.getMaxTime())) {
						it = mSpeechBubbles.erase(it);
					} else {
						++it;
					}
				}
			}
		}

		if(handleInput(frameTime))
			break;

		if(!mObserver && mSoldier->isDead()) {
			if(mRestarting) {
				mRestarting = false;
				setFocusSoldier();
			}
		}

		startFrame();
		drawTerrain();
		drawEntities();
		drawTexts();
		drawOverlays();
		finishFrame();
	}
}

void Driver::act(float time)
{
	if(handleLeaderCheck(time)) {
		std::cout << "Rank updated.\n";
	}

	if(mDigging) {
		if(!mDriving && !mPlayerControlVelocity.null()) {
			mDigging = false;
		} else {
			mSoldier->dig(time);
			return;
		}
	}

	mSoldier->handleEvents();
	Vector3 tot;
	Vector3 mousedir = getMousePositionOnField() - mSoldier->getPosition();

	if(!mDriving) {
		tot = defaultMovement(time);
		mSteering->accumulate(tot, mPlayerControlVelocity);
		turnTo(mousedir);
	} else {
		if(mPlayerControlVelocity.y) {
			Vector3 vec = mSoldier->getHeadingVector() * mPlayerControlVelocity.y;
			mSteering->accumulate(tot, vec);
		}
		if(mPlayerControlVelocity.x) {
			float rot(mPlayerControlVelocity.x);
			rot *= -0.05f;
			turnBy(rot);
			if(mSoldier->getHeadingVector().dot(mSoldier->getVelocity()) > 0.0f) {
				setVelocityToHeading();
			} else {
				mSoldier->setVelocityToNegativeHeading();
			}
		}
	}
	moveTo(tot, time, false);

	if(mShooting) {
		if(mSoldier->getCurrentWeapon() && mSoldier->getCurrentWeapon()->canShoot()) {
			mSoldier->getCurrentWeapon()->shoot(mWorld, mSoldier, mousedir);
		}
	}

	if(mRectangleFinished) {
		mRectangleFinished = false;
		if(!mObserver && mSoldier->getRank() == SoldierRank::Lieutenant &&
				mSelectedCommandee && mSoldier->canCommunicateWith(mSelectedCommandee)) {
			if(!mSelectedCommandee->giveAttackOrder(mDrawnRectangle)) {
				std::cout << "Group unable to comply.\n";
			}
		}
	}

	if(!mDesignatedDefendPosition.null()) {
		if(!mObserver && mSoldier->getRank() == SoldierRank::Sergeant &&
				mSelectedCommandee && mSoldier->canCommunicateWith(mSelectedCommandee)) {
			mSelectedCommandee->setDefendPosition(mDesignatedDefendPosition);
		}
		mDesignatedDefendPosition = Vector3();
	}
}

bool Driver::handleAttackOrder(const Rectangle& r)
{
	std::cout << "You should attack the area at " << r << "\n";
	return true;
}

bool Driver::handleAttackSuccess(SoldierPtr s, const Common::Rectangle& r)
{
	std::cout << "Attack to area " << r << " successful\n";
	return true;
}

void Driver::markArea(const Common::Color& c, const Common::Rectangle& r, bool onlyframes)
{
	mDebugSymbols.areas.push_back(DebugSymbolCollection::Area(c, r, onlyframes));
}

void Driver::addArrow(const Common::Color& c, const Common::Vector3& start, const Common::Vector3& end)
{
	mDebugSymbols.arrows.push_back(DebugSymbolCollection::Arrow(c, start, end));
}

void Driver::addMessage(const boost::shared_ptr<Soldier> s, const Common::Color& c, const char* text)
{
	if(s && s->getSideNum() != 0)
		return;

	char buf[256];
	snprintf(buf, 255, "%s: %s", mWorld->getCurrentTimeAsString().c_str(), text);
	buf[255] = 0;

	mInfoMessages[mNextInfoMessageIndex++] = InfoMessage(c, buf);
	if(mNextInfoMessageIndex >= mInfoMessages.size())
		mNextInfoMessageIndex = 0;
}

void Driver::say(boost::shared_ptr<Soldier> s, const char* msg)
{
	mSpeechBubbles[s] = SpeechBubble(msg);
}

void Driver::loadTextures()
{
	SDLSurface surfs[] = { SDLSurface("share/soldier1-n.png"),
		SDLSurface("share/soldier1-w.png"),
		SDLSurface("share/soldier1-s.png"),
		SDLSurface("share/soldier1-e.png"),
		};

	SDLSurface tanksurfs[] = { SDLSurface("share/tank-n.png"),
		SDLSurface("share/tank-w.png"),
		SDLSurface("share/tank-s.png"),
		SDLSurface("share/tank-e.png"),
	};

	for(int j = 0; j < NUM_SIDES; j++) {
		for(int k = 0; k < 4; k++) {
			{
				SDLSurface surf(surfs[k]);
				surf.mapPixelColor( [&] (const Color& c) { return mapSideColor(j == 0, c); } );
				mSoldierTexture[j][k] = boost::shared_ptr<Texture>(new Texture(surf, 0, 32));
			}

			{
				SDLSurface surf(tanksurfs[k]);
				surf.mapPixelColor( [&] (const Color& c) { return mapTankColor(j == 0, c); } );
				mTankTexture[j][k] = boost::shared_ptr<Texture>(new Texture(surf, 0, 0));
			}

			{
				SDLSurface surf(tanksurfs[k]);
				surf.mapPixelColor( [&] (const Color& c) { return mapDestroyedTankColor(j == 0, c); } );
				mDestroyedTankTexture[j][k] = boost::shared_ptr<Texture>(new Texture(surf, 0, 0));
			}
		}

		SDLSurface surf("share/soldier1-fallen.png");
		surf.mapPixelColor( [&] (const Color& c) { return mapSideColor(j == 0, c); } );
		mFallenSoldierTexture[j] = boost::shared_ptr<Texture>(new Texture(surf, 0, 32));
	}

	mGrassTexture = boost::shared_ptr<Texture>(new Texture("share/grass1.png", 0, 0));
	mSoldierShadowTexture = boost::shared_ptr<Texture>(new Texture("share/soldier1shadow.png", 0, 32));
	mTreeTexture = boost::shared_ptr<Texture>(new Texture("share/tree.png", 0, 0));
	mFoxholeTexture = boost::shared_ptr<Texture>(new Texture("share/foxhole.png", 0, 0));
	mTreeShadowTexture = mSoldierShadowTexture;
	mBrightSpot = boost::shared_ptr<Texture>(new Texture("share/spot.png", 0, 0));
	mWeaponPickupTextures[int(WeaponPickupTexture::Unknown)]      = boost::shared_ptr<Texture>(new Texture("share/question.png", 0, 0));
	mWeaponPickupTextures[int(WeaponPickupTexture::AssaultRifle)] = boost::shared_ptr<Texture>(new Texture("share/assaultrifle.png", 0, 0));
	mWeaponPickupTextures[int(WeaponPickupTexture::MachineGun)]   = boost::shared_ptr<Texture>(new Texture("share/machinegun.png", 0, 0));
	mWeaponPickupTextures[int(WeaponPickupTexture::Bazooka)]      = boost::shared_ptr<Texture>(new Texture("share/bazooka.png", 0, 0));
}

void Driver::loadFont()
{
	mFont = TTF_OpenFont("share/DejaVuSans.ttf", 12);
	if(!mFont) {
		fprintf(stderr, "Could not open font: %s\n", TTF_GetError());
		throw std::runtime_error("Loading font");
	}
}

Common::Color Driver::mapSideColor(bool first, const Common::Color& c)
{
	if(c.r == 240 && c.g == 240 && c.b == 0) {
		if(first) {
			return Common::Color::Red;
		} else {
			return Common::Color::Blue;
		}
	}

	if((c.r == 255) ||
			(c.r == 0 && c.g == 0 && c.b == 255)) {
		return Color(25, 68, 29);
	}

	return c;
}

Common::Color Driver::mapTankColor(bool first, const Common::Color& c)
{
	if((c.r + c.b) / 4 > c.g) {
		if(first) {
			Color r(c);
			r.b = r.g;
			return r;
		}
		else {
			Color r(c);
			r.r = r.g;
			return r;
		}
	}

	return c;
}

Common::Color Driver::mapDestroyedTankColor(bool first, const Common::Color& c)
{
	if((c.r + c.b) / 4 > c.g) {
		if(first) {
			Color r(c);
			r.b = r.g;
			return r;
		}
		else {
			Color r(c);
			r.r = r.g;
			return r;
		}
	}

	Color r(c);
	r.r /= 4;
	r.g /= 4;
	r.b /= 4;

	return r;
}

bool Driver::handleInput(float frameTime)
{
	bool quitting = false;
	SDL_Event event;
	memset(&event, 0, sizeof(event));
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym) {
					case SDLK_ESCAPE:
						quitting = true;
						break;

					case SDLK_MINUS:
					case SDLK_KP_MINUS:
						if(mTimeAcceleration > 1.0f)
							mTimeAcceleration = std::max(1.0f, mTimeAcceleration / 2.0f);
						break;

					case SDLK_PLUS:
					case SDLK_KP_PLUS:
						if(mTimeAcceleration < 128.0f)
							mTimeAcceleration = std::min(128.0f, mTimeAcceleration * 2.0f);
						break;

					case SDLK_PAGEDOWN:
						if(mFreeCamera)
							mScaleLevelVelocity = -1.0f; break;

					case SDLK_PAGEUP:
						if(mFreeCamera)
							mScaleLevelVelocity = 1.0f; break;

					case SDLK_w:
					case SDLK_UP:
						if(mFreeCamera)
							mCameraVelocity.y = -1.0f;
						else
							mPlayerControlVelocity.y = 1.0f;
						break;

					case SDLK_s:
					case SDLK_DOWN:
						if(mFreeCamera)
							mCameraVelocity.y = 1.0f;
						else
							mPlayerControlVelocity.y = -1.0f;
						break;

					case SDLK_d:
					case SDLK_RIGHT:
						if(mFreeCamera)
							mCameraVelocity.x = -1.0f;
						else
							mPlayerControlVelocity.x = 1.0f;
						break;

					case SDLK_a:
					case SDLK_LEFT:
						if(mFreeCamera)
							mCameraVelocity.x = 1.0f;
						else
							mPlayerControlVelocity.x = -1.0f;
						break;

					case SDLK_f:
						mFreeCamera = !mFreeCamera;
						break;

					case SDLK_x:
						if(!mObserver && mSoldier) {
							mSoldier->pruneCommandees();
							mSoldier->setLineFormation(10.0f);
						}
						break;

					case SDLK_c:
						if(!mObserver && mSoldier) {
							mSoldier->pruneCommandees();
							mSoldier->setColumnFormation(10.0f);
						}
						break;

					case SDLK_p:
					case SDLK_PAUSE:
						mPaused = !mPaused;
						break;

					case SDLK_v:
						if(!mObserver && mSoldier && !mSoldier->isDead() &&
								mSoldier->getLeader() && mSoldier->getAttackArea().w &&
								((mSoldier->getRank() == SoldierRank::Sergeant &&
								mSoldier->getAttackArea().pointWithin(mSoldier->getPosition().x,
									mSoldier->getPosition().y)) ||
								(mSoldier->getRank() == SoldierRank::Lieutenant &&
								allCommandeesDefending()))) {
							if(mSoldier->canCommunicateWith(mSoldier->getLeader())) {
								mSoldier->setDefending();
								if(!mSoldier->reportSuccessfulAttack()) {
									/* TODO */
								}
							} else {
								/* TODO */
							}
						}
						break;

					case SDLK_b:
						if(!mObserver && mSoldier->getWarriorType() == WarriorType::Soldier) {
							mDigging = !mDigging;
							if(mDigging)
								InfoChannel::getInstance()->say(mSoldier, "Digging a foxhole");
						}
						break;

					case SDLK_TAB:
						setFocusSoldier();
						break;

					case SDLK_1:
					case SDLK_2:
					case SDLK_3:
					case SDLK_4:
					case SDLK_5:
					case SDLK_6:
					case SDLK_7:
					case SDLK_8:
						if(!mObserver) {
							int k = event.key.keysym.sym - SDLK_1;
							mSoldier->switchWeapon(k);
						}
						break;

					case SDLK_F1:
					case SDLK_F2:
					case SDLK_F3:
					case SDLK_F4:
					case SDLK_F5:
					case SDLK_F6:
					case SDLK_F7:
					case SDLK_F8:
						if(!mObserver && mSoldier->getRank() > SoldierRank::Private) {
							int k = event.key.keysym.sym - SDLK_F1;
							int i = 0;
							for(auto c : mSoldier->getCommandees()) {
								if(i == k) {
									mSelectedCommandee = c;
									break;
								}
								i++;
							}
						}
						break;

					default:
						break;
				}
				break;

			case SDL_KEYUP:
				switch(event.key.keysym.sym) {
					case SDLK_PAGEUP:
					case SDLK_PAGEDOWN:
						if(mFreeCamera)
							mScaleLevelVelocity = 0.0f; break;

					case SDLK_w:
					case SDLK_s:
					case SDLK_UP:
					case SDLK_DOWN:
						if(mFreeCamera)
							mCameraVelocity.y = 0.0f;
						else
							mPlayerControlVelocity.y = 0.0f;
						break;

					case SDLK_a:
					case SDLK_d:
					case SDLK_RIGHT:
					case SDLK_LEFT:
						if(mFreeCamera)
							mCameraVelocity.x = 0.0f;
						else
							mPlayerControlVelocity.x = 0.0f;
						break;

					case SDLK_b:
						break;

					default:
						break;
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
				switch(event.button.button) {
					case SDL_BUTTON_LEFT:
						if(mSoldier->isDead())
							mRestarting = true;
						else
							mShooting = true;
						break;

					case SDL_BUTTON_RIGHT:
						if(!mObserver) {
							if(mSoldier->getRank() == SoldierRank::Lieutenant) {
								mCreatingRectangle = true;
								Common::Vector3 p = getMousePositionOnField();
								mDrawnRectangle.x = p.x;
								mDrawnRectangle.y = p.y;
								mDrawnRectangle.w = 0.0f;
								mDrawnRectangle.h = 0.0f;
							} else if(mSoldier->getRank() == SoldierRank::Sergeant) {
								mDesignatedDefendPosition = getMousePositionOnField();
							}
						}
						break;
				}
				break;

			case SDL_MOUSEBUTTONUP:
				switch(event.button.button) {
					case SDL_BUTTON_WHEELUP:
						if(mFreeCamera)
							mScaleLevel += 0.5f; break;
					case SDL_BUTTON_WHEELDOWN:
						if(mFreeCamera)
							mScaleLevel -= 0.5f; break;

					case SDL_BUTTON_LEFT:
						mShooting = false;
						break;

					case SDL_BUTTON_RIGHT:
						mCreatingRectangle = false;
						if(!mObserver && mSoldier->getRank() == SoldierRank::Lieutenant &&
								mSelectedCommandee &&
								mSoldier->canCommunicateWith(mSelectedCommandee)) {
							mRectangleFinished = true;
						}
						break;

					default:
						break;
				}
				break;

			case SDL_MOUSEMOTION:
				if(mCreatingRectangle) {
					Common::Vector3 p = getMousePositionOnField();
					mDrawnRectangle.w = p.x - mDrawnRectangle.x;
					mDrawnRectangle.h = p.y - mDrawnRectangle.y;
				}
				mCameraMouseOffset = (getMousePositionOnField() - mSoldier->getPosition()) * 0.45f;
				break;

			case SDL_QUIT:
				quitting = true;
				break;
			default:
				break;
		}
	}
	handleInputState(frameTime);
	return quitting;
}

void Driver::handleInputState(float frameTime)
{
	if(mFreeCamera) {
		mCamera -= mCameraVelocity * frameTime * 50.0f;
		mScaleLevel += mScaleLevelVelocity * frameTime * 10.0f;
		mScaleLevel = clamp(1.0f, mScaleLevel, 10.0f);
	}
	else if(mSoldier) {
		if(mObserver && mSoldier->isDead()) {
			setFocusSoldier();
			if(mSoldier->isDead()) {
				mFreeCamera = true;
			}
		}
		mCamera = mSoldier->getPosition() + mCameraMouseOffset;
		static const float scalechangevelocity = 0.9f;
		static const float scalecoefficient = 1.5f;
		float n;
		if(mSoldier->getCurrentWeapon())
			n = std::min(mSoldier->getCurrentWeapon()->getRange(), 0.5f * float(std::min(screenWidth, screenHeight)));
		else
			n = 50.0f;
		float d = clamp(std::min(n, 30.0f), mCameraMouseOffset.length() * mScaleLevel, n);
		float newScaleLevel = std::max(scalecoefficient * n / d, 300.0f * (1.0f / n));
		mScaleLevel = mScaleLevel * scalechangevelocity + newScaleLevel * (1.0f - scalechangevelocity);
	}
}

float Driver::restrictCameraCoordinate(float t, float w, float res)
{
	const float minX = w * -0.5f;
	const float maxX = w * 0.5f;
	const float minXCamPix = minX * mScaleLevel + res * 0.5f;
	const float maxXCamPix = maxX * mScaleLevel - res * 0.5f;
	const float minXCam = minXCamPix / mScaleLevel;
	const float maxXCam = maxXCamPix / mScaleLevel;
	if(minXCam > maxXCam) {
		return 0.0f;
	} else {
		return clamp(minXCam, t, maxXCam);
	}
}

void Driver::startFrame()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mCamera.x = restrictCameraCoordinate(mCamera.x, mWorld->getWidth(), screenWidth);
	mCamera.y = restrictCameraCoordinate(mCamera.y, mWorld->getHeight(), screenHeight);
}

void Driver::finishFrame()
{
	SDL_GL_SwapBuffers();
}

void Driver::drawTerrain()
{
	float pwidth = mWorld->getWidth();
	float pheight = mWorld->getHeight();
	SDL_utils::drawSprite(*mGrassTexture, Rectangle((-mCamera.x - pwidth) * mScaleLevel + screenWidth * 0.5f,
				(-mCamera.y - pheight) * mScaleLevel + screenHeight * 0.5f,
				mScaleLevel * pwidth * 2.0f,
				mScaleLevel * pheight * 2.0f),
			Rectangle(0, 0, 20, 20), 0.0f);
}

void Driver::drawTexts()
{
	if(mPaused) {
		drawOverlayText("Paused", 2.0f, Common::Color::White, 0.5f, 0.5f, true);
	}

	{
		// time
		drawOverlayText(mWorld->getCurrentTimeAsString().c_str(),
				1.0f, Common::Color::White,
				screenWidth - 100.0f, 10.0f, false, true);
	}

	if(fabs(mTimeAcceleration - 1.0f) > 0.1f) {
		// time acceleration
		char buf[128];
		snprintf(buf, 128, "%.1fx time", mTimeAcceleration);
		drawOverlayText(buf, 1.0f, Common::Color::White, screenWidth - 100.0f, 24.0f, false, true);
	}

	{
		// weapons
		int i = 0;
		for(auto w : mSoldier->getWeapons()) {
			drawOverlayText(w->getName(), 1.0f, w->getLoadTime() < 0.2f || w->canShoot() ?
					Common::Color::White : Common::Color::Black,
					40.0f, screenHeight - 100.0f - 15.0f * i, false, true);
			if(w == mSoldier->getCurrentWeapon()) {
				drawOverlayText("*", 1.0f, Common::Color::White, 30.0f, screenHeight - 100.0f - 15.0f * i, false, true);
			}
			i++;
		}
	}

	{
		// name and rank
		char buf[128];
		snprintf(buf, 127, "%s %s", Soldier::rankToString(mSoldier->getRank()), mSoldier->getName().c_str());
		buf[127] = 0;
		drawOverlayText(buf, 1.0f, getGroupRectangleColor(mSoldier, 1.0f),
				40.0f, screenHeight - 145.0f, false, true);
	}

	{
		// "n soldiers"
		for(int i = 0; i < NUM_SIDES; i++) {
			char alivebuf[128];
			Color c = i == 0 ? Color::Red : Color::Blue;
			int alive = mWorld->soldiersAlive(i);
			snprintf(alivebuf, 127, "%d soldier%s", alive, alive == 1 ? "" : "s");
			alivebuf[127] = 0;
			drawOverlayText(alivebuf, 1.0f, c,
					40.0f, screenHeight - 40.0f - 15.0f * (i + 2), false, true);
		}
	}

	{
		// soldier names
		if(mObserver) {
			for(auto s : mWorld->getSoldiersAt(mCamera, getDrawRadius())) {
				Color c;
				switch(s->getRank()) {
					case SoldierRank::Private:
						c = Color(255, 255, 255);
						break;

					case SoldierRank::Sergeant:
						c = Color(200, 200, 200);
						break;

					case SoldierRank::Lieutenant:
						c = Color(255, 240, 0);
						break;

					case SoldierRank::Captain:
						c = Color(207, 127, 50);
						break;
				}
				drawSoldierName(s, c);
			}
		} else {
			auto allseen = mSoldier->getSensorySystem()->getSoldiers();
			// self
			drawSoldierName(mSoldier, Color(192, 192, 192));

			if(mSoldier->getLeader()) {
				if(std::find(allseen.begin(), allseen.end(), mSoldier->getLeader()) != allseen.end()) {
					// leader
					drawSoldierName(mSoldier->getLeader(), Color(255, 215, 0));
				}

				for(auto s : mSoldier->getLeader()->getCommandees()) {
					if(s != mSoldier) {
						if(std::find(allseen.begin(), allseen.end(), s) != allseen.end()) {
							// peers
							drawSoldierName(s, Color(192, 192, 192));
						}
					}
				}
			}

			for(auto s : mSoldier->getCommandees()) {
				if(std::find(allseen.begin(), allseen.end(), s) != allseen.end()) {
					// commandees
					drawSoldierName(s, Color(207, 127, 50));
				}
			}
		}
	}

	switch(mWorld->teamWon()) {
		case -1:
		default:
			{
				if(!mObserver) {
					if(mSoldier->isDead()) {
						drawOverlayText("You're dead", 3.0f, Color::Red,
								0.5f, 0.45f, true);
					}
				}
			}
			break;

		case 0:
		case 1:
			{
				char buf[128];
				Color c = mWorld->teamWon() == 0 ? Color::Red : Color::Blue;
				memset(buf, 0, sizeof(buf));
				snprintf(buf, 127, "%s team wins", mWorld->teamWon() == 0 ? "Red" : "Blue");
				buf[127] = 0;
				drawOverlayText(buf, 3.0f, c, 0.5f, 0.45f, true);
			}
			break;

		case -2:
			drawOverlayText("Draw", 3.0f, Color::White, 0.5f, 0.45f, true);
			break;
	}

	if(mSoldier->getRank() >= SoldierRank::Sergeant) {
		// leader info
		float yp = 0.8f;
		float bright = 0.8f;
		for(auto s : mSoldier->getCommandees()) {
			Color c = getGroupRectangleColor(s, bright);
			bright -= 0.1f;

			char buf[128];
			buf[127] = 0;
			if(mSoldier->getRank() == SoldierRank::Sergeant) {
				if(s->getWarriorType() == WarriorType::Vehicle) {
					snprintf(buf, 127, "Vehicle %s", s->getName().c_str());
				} else {
					snprintf(buf, 127, "%s %s%s%s", Soldier::rankToString(s->getRank()),
							s->getName().c_str(),
							s->hasWeaponType("Bazooka") ? " (B)" : "",
							s->hasWeaponType("Machine Gun") ? " (MG)" : "");
				}
			} else {
				int numprivates = getNumberOfAvailableCommandees(s);
				snprintf(buf, 127, "%s %s (%d)", Soldier::rankToString(s->getRank()),
						s->getName().c_str(), numprivates + 1);
			}
			drawOverlayText(buf, 1.0f, c, 0.8f, yp, false);
			yp -= 0.04f;
		}
	}

	{
		// debug messages
		unsigned int i = mNextInfoMessageIndex; 
		float y = 0.16f;

		while(1) {
			if(mInfoMessages[i].mText.size()) {
				drawOverlayText(mInfoMessages[i].mText.c_str(), 1.0,
						mInfoMessages[i].mColor,
						0.05f, y, false, false);
				y -= 0.03f;
			}
			i++;
			if(i >= mInfoMessages.size())
				i = 0;
			if(i == mNextInfoMessageIndex)
				break;
		}
	}
}

void Driver::drawOverlays()
{
	{
		// goto-positions for the crew
		if(mSoldier->getRank() == SoldierRank::Private) {
			drawSoldierGotoMarker(mSoldier, false);
		} else if(mSoldier->getRank() == SoldierRank::Sergeant) {
			for(auto c : mSoldier->getCommandees()) {
				drawSoldierGotoMarker(c, true);
			}
		}
	}

	{
		// big picture
		if(mSoldier->getLeader() && mSoldier->getLeader()->getAttackArea().w) {
			drawRectangle(mSoldier->getLeader()->getAttackArea(), Common::Color::Yellow, 1.0f, true);
		}

		// group leader
		if(mSoldier->getAttackArea().w) {
			drawRectangle(mSoldier->getAttackArea(), Common::Color::White, 1.0f, true);
		}
	}

	{
		// platoon leader
		if(mCreatingRectangle) {
			drawRectangle(mDrawnRectangle, Common::Color::White, 1.0f, true);
		}

		float bright = 0.8f;
		for(auto s : mSoldier->getCommandees()) {
			drawRectangle(s->getAttackArea(), getGroupRectangleColor(s, bright), 1.0f, true);
			bright -= 0.1f;
		}
	}

	{
		// debug symbols
		for(auto a : mDebugSymbols.areas) {
			drawRectangle(a.r, a.c, 0.2f, a.onlyframes);
		}

		for(auto a : mDebugSymbols.arrows) {
			Vector3 arrow = a.end - a.start;
			Vector3 v1 = Math::rotate2D(arrow, QUARTER_PI);
			Vector3 v2 = Math::rotate2D(arrow, -QUARTER_PI);
			v1 *= -0.3f;
			v2 *= -0.3f;
			v1 += a.end;
			v2 += a.end;
			drawLine(a.start, a.end, a.c);
			drawLine(a.end, v1, a.c);
			drawLine(a.end, v2, a.c);
		}

		mDebugSymbols.clear();
	}
}

void Driver::drawRectangle(const Common::Rectangle& r,
		const Common::Color& c, float alpha, bool onlyframes)
{
	SDL_utils::drawRectangle((-mCamera.x + r.x) * mScaleLevel + screenWidth * 0.5f,
			(-mCamera.y + r.y) * mScaleLevel + screenHeight * 0.5f,
			(-mCamera.x + r.x + r.w) * mScaleLevel + screenWidth * 0.5f,
			(-mCamera.y + r.y + r.h) * mScaleLevel + screenHeight * 0.5f,
			c, alpha, onlyframes);
}

void Driver::drawLine(const Common::Vector3& p1, const Common::Vector3& p2, const Common::Color& c)
{
	Vector3 v1((-mCamera.x + p1.x) * mScaleLevel + screenWidth * 0.5f,
			(-mCamera.y + p1.y) * mScaleLevel + screenHeight * 0.5f,
			0.0f);
	Vector3 v2((-mCamera.x + p2.x) * mScaleLevel + screenWidth * 0.5f,
			(-mCamera.y + p2.y) * mScaleLevel + screenHeight * 0.5f,
			0.0f);
	SDL_utils::drawLine(v1, v2, c, 1.0f);
}

const boost::shared_ptr<Texture> Driver::soldierTexture(const SoldierPtr p, float& sxp, float& syp,
		float& xp, float& yp, float& scale)
{
	int sideIndex = p->getSide()->isFirst() ? 0 : 1;
	bool dead = p->isDead();
	bool soldier = p->getWarriorType() == WarriorType::Soldier;

	if(soldier) {
		scale = 2.0f;
		xp = -0.4f;
		yp = 0.0f;
		sxp = -0.4f;
		syp = -0.5f;
	} else {
		scale = 8.0f;
		xp = -0.5f;
		yp = -0.5f;
		sxp = -0.5f;
		syp = -0.7f;
	}

	if(dead && soldier) {
		sxp = -0.4f;
		syp = -0.1f;
		return mFallenSoldierTexture[sideIndex];
	}

	float r = p->getXYRotation();
	int dir = 3; // west
	if(r < QUARTER_PI && r >= -QUARTER_PI) {
		dir = 1; // east
	}
	else if(r < 3.0 * QUARTER_PI && r >= QUARTER_PI) {
		dir = 0; // north
	}
	else if(r >= -3.0 * QUARTER_PI && r < -QUARTER_PI) {
		dir = 2; // south
	}

	if(soldier) {
		return mSoldierTexture[sideIndex][dir];
	} else {
		if(dead) {
			return mDestroyedTankTexture[sideIndex][dir];
		} else {
			return mTankTexture[sideIndex][dir];
		}
	}
}

void Driver::drawEntities()
{
	static const float treeScale = 3.0f;
	const auto soldiervec = mObserver || mSoldier->isDead() ?
		mWorld->getSoldiersAt(mCamera, getDrawRadius()) :
		mSoldier->getSensorySystem()->getSoldiers();

	std::set<Sprite> soldiers;
	includeSoldierSprite(soldiers, mSoldier);

	if(!mObserver) {
		for(auto s : mSoldier->getCommandees()) {
			if(mSoldier->canCommunicateWith(s)) {
				bool addbrightspot = s == mSelectedCommandee;
				includeSoldierSprite(soldiers, s, addbrightspot);

				for(auto p : s->getSensorySystem()->getSoldiers()) {
					includeSoldierSprite(soldiers, p);
				}

				for(auto c : s->getCommandees()) {
					if(s->canCommunicateWith(c)) {
						includeSoldierSprite(soldiers, c, addbrightspot);

						for(auto p : c->getSensorySystem()->getSoldiers()) {
							includeSoldierSprite(soldiers, p);
						}
					}
				}
			}
		}

		auto l = mSoldier->getLeader();
		if(l && mSoldier->canCommunicateWith(l)) {
			includeSoldierSprite(soldiers, l);
			for(auto s : l->getCommandees()) {
				if(mSoldier->canCommunicateWith(s)) {
					includeSoldierSprite(soldiers, s);
				}
			}
		}
	}

	for(auto s : soldiervec) {
		includeSoldierSprite(soldiers, s);
	}

	std::function<bool (const Vector3&)> observefunc;
	std::set<SoldierPtr> comrades;

	if(mObserver) {
		const Vector3& observerpos = mCamera;
		float observerdist = getDrawRadius();
		float observerdist2 = observerdist * observerdist;
		observefunc = [&](const Vector3& v) -> bool {
			return observerpos.distance2(v) <= observerdist2;
		};
	} else {
		float observerdist = mWorld->getShootSoundHearingDistance();
		float observerdist2 = observerdist * observerdist;

		comrades.insert(mSoldier);

		for(auto s : mSoldier->getCommandees()) {

			if(mSoldier->canCommunicateWith(s)) {
				comrades.insert(s);

				for(auto c : s->getCommandees()) {
					if(s->canCommunicateWith(c)) {
						comrades.insert(c);
					}
				}
			}
		}

		observefunc = [&](const Vector3& v) -> bool {
			for(auto s : comrades) {
				if(s->getPosition().distance2(v) <= observerdist2)
					return true;
			}

			return false;
		};
	}

	std::vector<Sprite> sprites;
	for(auto s : soldiers) {
		sprites.push_back(s);
	}

	auto foxholes = mObserver ? mWorld->getFoxholesAt(mCamera, getDrawRadius()) :
		mSoldier->getSensorySystem()->getFoxholes();

	for(auto f : foxholes) {
		sprites.push_back(Sprite(f->getPosition(), SpriteType::Foxhole,
					4.0f, mFoxholeTexture, boost::shared_ptr<Texture>(),
					-0.5f, -0.5f, 0.0f, 0.0f, clamp(0.0f, f->getDepth(), 1.0f)));
	}

	auto trees = mWorld->getTreesAt(mCamera, getDrawRadius());

	for(auto t : trees) {
		sprites.push_back(Sprite(t->getPosition(), SpriteType::Tree,
					t->getRadius() * treeScale, mTreeTexture, mTreeShadowTexture, -0.5f, -0.5f,
					-0.5f, -0.8f));
	}

	for(auto b : mWorld->getBulletsAt(mCamera, getDrawRadius())) {
		if(!observefunc(b->getPosition())) {
			continue;
		}

		float scale = b->getWeapon()->getDamageAgainstLightArmor() > 0.0f ? 5.0f : 1.0f;
		sprites.push_back(Sprite(b->getPosition(), SpriteType::Bullet,
					scale,
					boost::shared_ptr<Texture>(), boost::shared_ptr<Texture>(),
					0.0f, 0.5f / scale,
					-0.8f / scale, -1.0f / scale));
	}

	auto triggers = mWorld->getTriggerSystem().getTriggers();
	for(auto t : triggers) {
		if(!observefunc(t->getPosition())) {
			continue;
		}

		const char* tn = t->getName();
		WeaponPickupTexture tex = WeaponPickupTexture::Unknown;
		if(!strncmp(tn, "WeaponPickup", sizeof("WeaponPickup") - 1)) {
			tn += sizeof("WeaponPickup") - 1;
			if(!strcmp(tn, "Assault Rifle")) {
				tex = WeaponPickupTexture::AssaultRifle;
			}
			else if(!strcmp(tn, "Machine Gun")) {
				tex = WeaponPickupTexture::MachineGun;
			}
			else if(!strcmp(tn, "Bazooka")) {
				tex = WeaponPickupTexture::Bazooka;
			}
			sprites.push_back(Sprite(t->getPosition(), SpriteType::WeaponPickup,
						2.0f,
						mWeaponPickupTextures[int(tex)], boost::shared_ptr<Texture>(),
						-0.5f, -0.5f,
						0.0f, 0.0f));
		}
	}

	for(auto s : sprites) {
		if(!s.mShadowTexture && s.mSpriteType != SpriteType::Bullet)
			continue;

		const Vector3& v(s.mPosition);
		Rectangle r = Rectangle((-mCamera.x + v.x + s.mSXP * s.mScale + v.z * 0.15f * s.mScale) * mScaleLevel + screenWidth * 0.5f,
				(-mCamera.y + v.y + s.mSYP * s.mScale - v.z * 0.20f * s.mScale) * mScaleLevel + screenHeight * 0.5f,
				mScaleLevel * s.mScale, mScaleLevel * s.mScale);

		if(s.mSpriteType != SpriteType::Bullet) {
			SDL_utils::drawSprite(*s.mShadowTexture,
					r,
					Rectangle(1, 1, -1, -1), 0.0f, s.mAlpha);
		} else {
			SDL_utils::drawPoint(Vector3(r.x, r.y, 0.0f), s.mScale, Color::Black);
		}
	}

	std::sort(sprites.begin(), sprites.end());

	for(auto s : sprites) {
		if(!s.mTexture && s.mSpriteType != SpriteType::Bullet)
			continue;

		const Vector3& v(s.mPosition);
		Rectangle r = Rectangle((-mCamera.x + v.x + s.mXP * s.mScale) * mScaleLevel + screenWidth * 0.5f,
				(-mCamera.y + v.y + s.mYP * s.mScale + v.z * 0.3f * s.mScale) * mScaleLevel + screenHeight * 0.5f,
				mScaleLevel * s.mScale, mScaleLevel * s.mScale);

		if(s.mSpriteType != SpriteType::Bullet) {
			SDL_utils::drawSprite(*s.mTexture, r,
					Rectangle(1, 1, -1, -1), 0.0f, s.mAlpha);
		} else {
			SDL_utils::drawPoint(Vector3(r.x, r.y, 0.0f), s.mScale, Color::White);
		}

#if 0
		SDL_utils::drawCircle((-mCamera.x + v.x) * mScaleLevel + screenWidth * 0.5f,
				(-mCamera.y  + v.y) * mScaleLevel + screenHeight * 0.5f,
				mScaleLevel * s.mScale / treeScale);
#endif
	}
}

void Driver::setFocusSoldier()
{
	const auto soldiers = mWorld->getSoldiersAt(mCamera, mWorld->getWidth());
	std::vector<SoldierPtr> soldiercandidates;
	for(auto s : soldiers) {
		if(s->getSideNum() == 0 && !s->isDead() && !s->isDictator() &&
				(s->getRank() == mSoldierRank)) {
			soldiercandidates.push_back(s);
		}
	}

	if(soldiercandidates.empty()) {
		// try again without rank preference
		for(auto s : soldiers) {
			if(s->getSideNum() == 0 && !s->isDead() && !s->isDictator()) {
				soldiercandidates.push_back(s);
			}
		}
	}

	if(soldiercandidates.empty())
		return;

	unsigned int i = Random::uniform() * soldiercandidates.size();
	i = clamp((unsigned int)0, i, (unsigned int)soldiercandidates.size() - 1);

	auto s = soldiercandidates[i];
	mSoldier = s;
	if(!mObserver) {
		mDriving = s->getWarriorType() == WarriorType::Vehicle;
		mDigging = false;
		setSoldier(mSoldier);
		if(mSoldier->getRank() == SoldierRank::Lieutenant && mSoldier->getCommandees().size() > 0) {
			mSelectedCommandee = *mSoldier->getCommandees().begin();
		}
	}
}

Vector3 Driver::getMousePositionOnField() const
{
	int xp, yp;
	float x, y;
	SDL_GetMouseState(&xp, &yp);
	yp = screenHeight - yp;

	x = float(xp) / mScaleLevel + mCamera.x - (screenWidth / (2.0f * mScaleLevel));
	y = float(yp) / mScaleLevel + mCamera.y - (screenHeight / (2.0f * mScaleLevel));

	return Vector3(x, y, 0);
}

void Driver::drawText(const char* text, float size, const Common::Color& c,
		const Common::Vector3& pos, bool centered)
{
	SDL_utils::drawText(mTextMap, mFont, mCamera, mScaleLevel, screenWidth, screenHeight,
			pos.x,
			pos.y,
			FontConfig(text, c, size),
			false, centered);
}

void Driver::drawSoldierName(const SoldierPtr s, const Common::Color& c)
{
	auto pos(s->getPosition());
	pos.y += 2.0f;
	drawText(s->getName().c_str(), 0.08f, c, pos, true);

	if(!s->isDead()) {
		// speech bubble
		auto it = mSpeechBubbles.find(s);
		if(it != mSpeechBubbles.end()) {
			auto& s = it->second.mText;
			auto pos(it->first->getPosition());
			pos.y += 3.0f;
			drawText(s.c_str(), 0.4f, Common::Color::White, pos, true);
		}
	}

}

void Driver::drawSoldierGotoMarker(const SoldierPtr s, bool alwaysdraw)
{
	auto leader = s->getLeader();
	if(leader) {
		auto& offset = s->getFormationOffset();
		if(!offset.null()) {
			Vector3 rotatedOffset = Math::rotate2D(offset, leader->getXYRotation());
			rotatedOffset += leader->getPosition();
			if(alwaysdraw || rotatedOffset.distance2(s->getPosition()) > 8.0f)
				drawText("x", 0.32f, Common::Color::White, rotatedOffset, true);
		} else {
			auto& defpos = s->getDefendPosition();
			if(!defpos.null()) {
				if(alwaysdraw || defpos.distance2(s->getPosition()) > 8.0f)
					drawText("x", 0.40f, Common::Color::White, defpos, true);
			}
		}
	}
}

float Driver::getDrawRadius() const
{
	return 10.0f + screenWidth * 0.5f / mScaleLevel;
}

void Driver::drawOverlayText(const char* text, float size, const Common::Color& c,
		float x, float y, bool centered, bool pixelcoords)
{
	int xv = pixelcoords ? x : screenWidth * x;
	int yv = pixelcoords ? y : screenHeight * y;
	SDL_utils::drawText(mTextMap, mFont, mCamera, mScaleLevel, screenWidth, screenHeight,
			xv, yv, FontConfig(text, c, size),
			true, centered);
}

Common::Color Driver::getGroupRectangleColor(const SoldierPtr commandee, float brightness)
{
	Color c;
	if(!mSoldier->canCommunicateWith(commandee)) {
		c = Color::Black;
	} else if(commandee->hasEnemyContact()) {
		c = Color::Red;
	} else if(commandee->defending()) {
		c = Color::White;
	} else {
		c = Color::Yellow;
	}

	if(commandee != mSelectedCommandee) {
		c.r *= brightness;
		c.g *= brightness;
		c.b *= brightness;
	}

	return c;
}

int Driver::getNumberOfAvailableCommandees(const SoldierPtr p)
{
	int num = 0;
	auto vis = p->getSensorySystem()->getSoldiers();

	for(auto s : p->getCommandees()) {
		if(p->canCommunicateWith(s)) {
			num++;
		}
	}

	return num;
}

void Driver::includeSoldierSprite(std::set<Sprite>& sprites, const SoldierPtr s, bool addbrightspot)
{
	float xp, yp, sxp, syp;
	float scale;
	boost::shared_ptr<Texture> t = soldierTexture(s, sxp, syp, xp, yp, scale);
	sprites.insert(Sprite(s->getPosition(), SpriteType::Soldier, scale, t, mSoldierShadowTexture, xp, yp,
				sxp, syp));
	if(addbrightspot) {
		Vector3 p = s->getPosition();
		p.y -= 0.001f;
		sprites.insert(Sprite(p, SpriteType::BrightSpot, scale, mBrightSpot,
					boost::shared_ptr<Common::Texture>(), xp, yp,
					0.0f, 0.0f, 0.5f));
	}
}

bool Driver::allCommandeesDefending() const
{
	for(auto s : mSoldier->getCommandees()) {
		if(!s->defending())
			return false;
	}
	return true;
}

}

