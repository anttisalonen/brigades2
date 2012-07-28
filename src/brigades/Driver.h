#ifndef BRIGADES_DRIVER_H
#define BRIGADES_DRIVER_H

#include <boost/shared_ptr.hpp>

#include <SDL.h>
#include <SDL_ttf.h>

#include "common/Clock.h"
#include "common/Texture.h"
#include "common/Color.h"
#include "common/FontConfig.h"

#include "World.h"


namespace Brigades {

class Driver {

	public:
		Driver(WorldPtr w);
		void run();

	private:
		void loadTextures();
		void loadFont();
		Common::Color mapSideColor(bool first, const Common::Color& c);
		bool handleInput(float frameTime);
		void handleInputState(float frameTime);
		float restrictCameraCoordinate(float t, float w);
		void startFrame();
		void finishFrame();
		void drawTerrain();
		void drawTexts();
		const boost::shared_ptr<Common::Texture> soldierTexture(const SoldierPtr p);
		void drawSoldiers();
		void setFocusSoldier();

		WorldPtr mWorld;
		Common::Clock mClock;
		SDL_Surface* mScreen;
		TTF_Font* mFont;
		bool mPaused;
		float mScaleLevel;
		float mScaleLevelVelocity;
		bool mFreeCamera;
		Vector3 mCamera;
		Vector3 mCameraVelocity;
		Vector3 mPlayerControlVelocity;
		boost::shared_ptr<Common::Texture> mSoldierTexture[NUM_SIDES][4];
		boost::shared_ptr<Common::Texture> mSoldierShadowTexture;
		boost::shared_ptr<Common::Texture> mGrassTexture;
		Common::TextMap mTextMap;
		SoldierPtr mFocusSoldier;
};

typedef boost::shared_ptr<Driver> DriverPtr;

};

#endif

