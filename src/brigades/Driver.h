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

enum class SpriteType {
	Soldier,
	Tree,
};

struct Sprite {
	Sprite(boost::shared_ptr<Common::Entity> e, SpriteType t, float scale,
			boost::shared_ptr<Common::Texture> texture,
			boost::shared_ptr<Common::Texture> shadow, float xp, float yp,
			float sxp, float syp)
		: mEntity(e), mSpriteType(t), mScale(scale), mTexture(texture), mShadowTexture(shadow),
		mXP(xp), mYP(yp), mSXP(sxp), mSYP(syp) { }
	boost::shared_ptr<Common::Entity> mEntity;
	SpriteType mSpriteType;
	float mScale;
	boost::shared_ptr<Common::Texture> mTexture;
	boost::shared_ptr<Common::Texture> mShadowTexture;
	float mXP;
	float mYP;
	float mSXP;
	float mSYP;
	bool operator<(const Sprite& s1) const;
};

class Driver {
	public:
		Driver(WorldPtr w);
		void init();
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
		void drawEntities();
		void setFocusSoldier();

		WorldPtr mWorld;
		Common::Clock mClock;
		SDL_Surface* mScreen;
		TTF_Font* mFont;
		bool mPaused;
		float mScaleLevel;
		float mScaleLevelVelocity;
		bool mFreeCamera;
		Common::Vector3 mCamera;
		Common::Vector3 mCameraVelocity;
		Common::Vector3 mPlayerControlVelocity;
		boost::shared_ptr<Common::Texture> mSoldierTexture[NUM_SIDES][4];
		boost::shared_ptr<Common::Texture> mSoldierShadowTexture;
		boost::shared_ptr<Common::Texture> mGrassTexture;
		boost::shared_ptr<Common::Texture> mTreeTexture;
		boost::shared_ptr<Common::Texture> mTreeShadowTexture;
		Common::TextMap mTextMap;
		SoldierPtr mFocusSoldier;
};

typedef boost::shared_ptr<Driver> DriverPtr;

};

#endif

