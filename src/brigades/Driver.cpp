#include "Driver.h"

#include "common/Rectangle.h"
#include "common/SDL_utils.h"

using namespace Common;

namespace Brigades {

static int screenWidth = 800;
static int screenHeight = 600;

Driver::Driver(WorldPtr w)
	: mWorld(w),
	mPaused(false),
	mScaleLevel(11.5f),
	mScaleLevelVelocity(0.0f),
	mFreeCamera(false)
{
	mScreen = SDL_utils::initSDL(screenWidth, screenHeight, "Brigades");

	loadTextures();
	loadFont();
	SDL_utils::setupOrthoScreen(screenWidth, screenHeight);

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

		startFrame();
		drawTerrain();
		drawSoldiers();
		drawTexts();
		finishFrame();
	}
}

void Driver::loadTextures()
{
	SDLSurface surfs[] = { SDLSurface("share/soldier1-n.png"),
		SDLSurface("share/soldier1-w.png"),
		SDLSurface("share/soldier1-s.png"),
		SDLSurface("share/soldier1-e.png"),
		};

	int i = 0;
	for(auto& s : surfs) {
		for(int j = 0; j < 2; j++) {
			SDLSurface surf(s);
			surf.mapPixelColor( [&] (const Color& c) { return mapSideColor(j == 0, c); } );
			mSoldierTexture[j][i] = boost::shared_ptr<Texture>(new Texture(surf, 0, 32));
		}
		i++;
	}

	mGrassTexture = boost::shared_ptr<Texture>(new Texture("share/grass1.png", 0, 0));
	mSoldierShadowTexture = boost::shared_ptr<Texture>(new Texture("share/soldier1shadow.png", 0, 32));
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
	if(c.r == 255 && c.b == 255) {
		if(first) {
			return Common::Color::Red;
		} else {
			return Common::Color::Blue;
		}
	}

	return c;
}

bool Driver::handleInput(float frameTime)
{
	bool quitting = false;
	SDL_Event event;
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

					case SDLK_p:
					case SDLK_PAUSE:
						mPaused = !mPaused;
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

			case SDL_MOUSEBUTTONUP:
				switch(event.button.button) {
					case SDL_BUTTON_WHEELUP:
						mScaleLevel += 4.0f; break;
					case SDL_BUTTON_WHEELDOWN:
						mScaleLevel -= 4.0f; break;
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
		mCamera -= mCameraVelocity * frameTime * 10.0f;
	}
	else if(mFocusSoldier) {
		mCamera = mFocusSoldier->getPosition();
	}
	mScaleLevel += mScaleLevelVelocity * frameTime * 10.0f;
	mScaleLevel = clamp(10.0f, mScaleLevel, 20.0f);
}

float Driver::restrictCameraCoordinate(float t, float w)
{
	const float minX = w * -0.5f - 5.0f;
	const float maxX = w * 0.5f + 5.0f;
	const float minXCamPix = minX * mScaleLevel + screenWidth * 0.5f;
	const float maxXCamPix = maxX * mScaleLevel - screenWidth * 0.5f;
	const float minXCam = minXCamPix / mScaleLevel;
	const float maxXCam = maxXCamPix / mScaleLevel;
	if(minXCam > maxXCam)
		return 0.0f;
	else
		return clamp(minXCam, t, maxXCam);
}

void Driver::startFrame()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mCamera.x = restrictCameraCoordinate(mCamera.x, mWorld->getWidth());
	mCamera.y = restrictCameraCoordinate(mCamera.y, mWorld->getWidth());
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
}

const boost::shared_ptr<Texture> Driver::soldierTexture(const SoldierPtr p)
{
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

	return mSoldierTexture[p->getSide()->isFirst() ? 0 : 1][dir];
}

void Driver::drawSoldiers()
{
	const auto soldiers = mWorld->getSoldiersAt(mCamera, 10.0f);
	for(auto s : soldiers) {
		const Vector3& v(s->getPosition());

		SDL_utils::drawSprite(*mSoldierShadowTexture,
				Rectangle((-mCamera.x + v.x - 0.8f + v.z * 0.3f) * mScaleLevel + screenWidth * 0.5f,
					(-mCamera.y + v.y - 0.8f - v.z * 0.4f) * mScaleLevel + screenHeight * 0.5f,
					mScaleLevel * 2.0f, mScaleLevel * 2.0f),
				Rectangle(1, 1, -1, -1), 0.0f);

		SDL_utils::drawSprite(*soldierTexture(s),
				Rectangle((-mCamera.x + v.x - 0.8f) * mScaleLevel + screenWidth * 0.5f,
					(-mCamera.y + v.y + v.z * 0.6f) * mScaleLevel + screenHeight * 0.5f,
					mScaleLevel * 2.0f, mScaleLevel * 2.0f),
				Rectangle(1, 1, -1, -1), 0.0f);
	}
}

void Driver::setFocusSoldier()
{
	const auto soldiers = mWorld->getSoldiersAt(mCamera, 1000.0f);
	if(!soldiers.empty())
		mFocusSoldier = soldiers[0];
}

}
