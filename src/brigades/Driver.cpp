#include <string.h>

#include "Driver.h"
#include "SensorySystem.h"

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

Driver::Driver(WorldPtr w, bool observer)
	: mWorld(w),
	mPaused(false),
	mScaleLevel(11.5f),
	mScaleLevelVelocity(0.0f),
	mFreeCamera(false),
	mSoldierVisible(false),
	mObserver(observer),
	mShooting(false),
	mRestarting(false),
	mDriving(false)
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

		if(!mPaused) {
			mWorld->update(frameTime);
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
		finishFrame();
	}
}

void Driver::act(float time)
{
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
			setVelocityToHeading();
		}
	}
	moveTo(tot, time, false);

	if(mShooting) {
		if(mSoldier->getCurrentWeapon() && mSoldier->getCurrentWeapon()->canShoot()) {
			mSoldier->getCurrentWeapon()->shoot(mWorld, mSoldier, mousedir);
		}
	}
}

bool Driver::handleAttackOrder(const Rectangle& r)
{
	std::cout << "You should attack the area at " << r << "\n";
	return true;
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
	mTreeShadowTexture = mSoldierShadowTexture;
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
					case SDLK_PAGEDOWN:
						mScaleLevelVelocity = -1.0f; break;
					case SDLK_PLUS:
					case SDLK_KP_PLUS:
					case SDLK_PAGEUP:
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
								mSoldier->getRank() == SoldierRank::Sergeant &&
								mSoldier->getLeader() && mSoldier->getAttackArea().w &&
								mSoldier->getAttackArea().pointWithin(mSoldier->getPosition().x,
									mSoldier->getPosition().y)) {
							if(mSoldier->canCommunicateWith(mSoldier->getLeader())) {
								mSoldier->setDefending();
								mSoldier->getLeader()->reportSuccessfulAttack(mSoldier->getAttackArea());
							} else {
								/* TODO */
							}
						}
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

					default:
						break;
				}
				break;

			case SDL_KEYUP:
				switch(event.key.keysym.sym) {
					case SDLK_MINUS:
					case SDLK_KP_MINUS:
					case SDLK_PLUS:
					case SDLK_KP_PLUS:
					case SDLK_PAGEUP:
					case SDLK_PAGEDOWN:
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
				}
				break;

			case SDL_MOUSEBUTTONUP:
				switch(event.button.button) {
					case SDL_BUTTON_WHEELUP:
						mScaleLevel += 4.0f; break;
					case SDL_BUTTON_WHEELDOWN:
						mScaleLevel -= 4.0f; break;

					case SDL_BUTTON_LEFT:
						mShooting = false;
						break;

					default:
						break;
				}
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
	}
	else if(mSoldier) {
		if(mObserver && mSoldier->isDead()) {
			setFocusSoldier();
			if(mSoldier->isDead()) {
				mFreeCamera = true;
			}
		}
		mCamera = mSoldier->getPosition();
	}
	mScaleLevel += mScaleLevelVelocity * frameTime * 10.0f;
	float maxScale = mObserver ? 5.0f : mSoldier->getMaxSpeed() ? 100.0f / mSoldier->getMaxSpeed() : 5.0f;
	mScaleLevel = clamp(clamp(5.0f, maxScale, 20.0f), mScaleLevel, 20.0f);
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
		SDL_utils::drawText(mTextMap, mFont, mCamera, mScaleLevel, screenWidth, screenHeight,
				screenWidth / 2, screenHeight / 2, FontConfig("Paused", Color(255, 255, 255), 2.0f),
				true, true);
	}

	{
		int i = 0;
		for(auto w : mSoldier->getWeapons()) {
			SDL_utils::drawText(mTextMap, mFont, mCamera, mScaleLevel, screenWidth, screenHeight,
					40.0f, screenHeight - 100.0f - 15.0f * i,
					FontConfig(w->getName(), Color(255, 255, 255), 1.0f),
					true, false);
			if(w == mSoldier->getCurrentWeapon()) {
				SDL_utils::drawText(mTextMap, mFont, mCamera, mScaleLevel, screenWidth, screenHeight,
						30.0f, screenHeight - 100.0f - 15.0f * i,
						FontConfig("*", Color(255, 255, 255), 1.0f),
						true, false);
			}
			i++;
		}
	}

	{
		for(int i = 0; i < NUM_SIDES; i++) {
			char alivebuf[128];
			memset(alivebuf, 0, sizeof(alivebuf));
			Color c = i == 0 ? Color::Red : Color::Blue;
			int alive = mWorld->soldiersAlive(i);
			snprintf(alivebuf, 127, "%d soldier%s", alive, alive == 1 ? "" : "s");
			alivebuf[127] = 0;
			SDL_utils::drawText(mTextMap, mFont, mCamera, mScaleLevel, screenWidth, screenHeight,
					40.0f, screenHeight - 40.0f - 15.0f * (i + 2), FontConfig(alivebuf, c, 1.0f),
					true, false);
		}
	}

	switch(mWorld->teamWon()) {
		case -1:
		default:
			{
				if(!mObserver) {
					if(mSoldier->isDead()) {
						SDL_utils::drawText(mTextMap, mFont, mCamera, mScaleLevel, screenWidth, screenHeight,
								screenWidth * 0.5f, screenHeight * 0.5f - 40.0f,
							       	FontConfig("You're dead", Color::Red, 3.0f),
								true, true);
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
				SDL_utils::drawText(mTextMap, mFont, mCamera, mScaleLevel, screenWidth, screenHeight,
						screenWidth * 0.5f, screenHeight * 0.5f - 40.0f, FontConfig(buf, c, 3.0f),
						true, true);
			}
			break;

		case -2:
			SDL_utils::drawText(mTextMap, mFont, mCamera, mScaleLevel, screenWidth, screenHeight,
					screenWidth * 0.5f, screenHeight * 0.5f - 40.0f, FontConfig("Draw", Color::White, 3.0f),
					true, true);
			break;

	}

	{
		if(mSoldier->getAttackArea().w) {
			SDL_utils::drawRectangle((-mCamera.x + mSoldier->getAttackArea().x) * mScaleLevel + screenWidth * 0.5f,
					(-mCamera.y + mSoldier->getAttackArea().y) * mScaleLevel + screenHeight * 0.5f,
					(-mCamera.x + mSoldier->getAttackArea().x + mSoldier->getAttackArea().w) * mScaleLevel + screenWidth * 0.5f,
					(-mCamera.y + mSoldier->getAttackArea().y + mSoldier->getAttackArea().h) * mScaleLevel + screenHeight * 0.5f);
		}
	}
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
	const auto soldiers = mObserver || mSoldier->isDead() ?
		mWorld->getSoldiersAt(mCamera, 10.0f) :
		mSoldier->getSensorySystem()->getSoldiers();

	std::vector<Sprite> sprites;
	for(auto s : soldiers) {
		float xp, yp, sxp, syp;
		float scale;
		boost::shared_ptr<Texture> t = soldierTexture(s, sxp, syp, xp, yp, scale);
		sprites.push_back(Sprite(s->getPosition(), SpriteType::Soldier, scale, t, mSoldierShadowTexture, xp, yp,
					sxp, syp));
	}

	auto trees = mWorld->getTreesAt(mCamera, 10.0f);
	for(auto t : trees) {
		sprites.push_back(Sprite(t->getPosition(), SpriteType::Tree,
					t->getRadius() * treeScale, mTreeTexture, mTreeShadowTexture, -0.5f, -0.5f,
					-0.5f, -0.8f));
	}

	auto bullets = mWorld->getBulletsAt(mCamera, 10.0f);
	for(auto b : bullets) {
		float scale = b->getWeapon()->getDamageAgainstLightArmor() > 0.0f ? 5.0f : 1.0f;
		sprites.push_back(Sprite(b->getPosition(), SpriteType::Bullet,
					scale,
					boost::shared_ptr<Texture>(), boost::shared_ptr<Texture>(),
					0.0f, 0.5f / scale,
					-0.8f / scale, -1.0f / scale));
	}


	auto triggers = mWorld->getTriggerSystem().getTriggers();
	for(auto t : triggers) {
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
					Rectangle(1, 1, -1, -1), 0.0f);
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
					Rectangle(1, 1, -1, -1), 0.0f);
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
	const auto soldiers = mWorld->getSoldiersAt(mCamera, 1000.0f);
	for(auto s : soldiers) {
		if(s->getSideNum() == 0 && !s->isDead() && !s->isDictator() && s->getRank() < SoldierRank::Lieutenant) {
			mSoldier = s;
			if(!mObserver) {
				mDriving = s->getWarriorType() == WarriorType::Vehicle;
				setSoldier(mSoldier);
				return;
			}
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

}

