#ifndef BRIGADES_DRIVER_H
#define BRIGADES_DRIVER_H

#include <boost/shared_ptr.hpp>

#include <SDL.h>
#include <SDL_ttf.h>

#include "common/Clock.h"
#include "common/Texture.h"
#include "common/Color.h"
#include "common/FontConfig.h"
#include "common/Rectangle.h"

#include "World.h"
#include "DebugOutput.h"


namespace Brigades {

enum class SpriteType {
	Soldier,
	Tree,
	Bullet,
	WeaponPickup
};

struct Sprite {
	Sprite(const Common::Vector3& pos, SpriteType t, float scale,
			boost::shared_ptr<Common::Texture> texture,
			boost::shared_ptr<Common::Texture> shadow, float xp, float yp,
			float sxp, float syp)
		: mPosition(pos), mSpriteType(t), mScale(scale), mTexture(texture), mShadowTexture(shadow),
		mXP(xp), mYP(yp), mSXP(sxp), mSYP(syp) { }
	Common::Vector3 mPosition;
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

enum class WeaponPickupTexture {
	Unknown,
	AssaultRifle,
	MachineGun,
	Bazooka,
	END,
};

struct DebugSymbolCollection {
	struct Area {
		Area(const Common::Color& c_, const Common::Rectangle& r_, bool onlyframes_)
			: c(c_), r(r_), onlyframes(onlyframes_) { }
		Common::Color c;
		Common::Rectangle r;
		bool onlyframes;
	};

	struct Arrow {
		Arrow(const Common::Color& c_, const Common::Vector3& start_, const Common::Vector3& end_)
			: c(c_), start(start_), end(end_) { }
		Common::Color c;
		Common::Vector3 start;
		Common::Vector3 end;
	};

	std::vector<Area> areas;
	std::vector<Arrow> arrows;

	void clear() {
		areas.clear();
		arrows.clear();
	}
};

class Driver : public SoldierController, public DebugOutput {
	public:
		Driver(WorldPtr w, bool observer, SoldierRank r);
		void init();
		void run();
		void act(float time) override;
		bool handleAttackOrder(const Common::Rectangle& r) override;
		void markArea(const Common::Color& c, const Common::Rectangle& r, bool onlyframes);
		void addArrow(const Common::Color& c, const Common::Vector3& start, const Common::Vector3& arrow);

	private:
		void loadTextures();
		void loadFont();
		Common::Color mapSideColor(bool first, const Common::Color& c);
		Common::Color mapTankColor(bool first, const Common::Color& c);
		Common::Color mapDestroyedTankColor(bool first, const Common::Color& c);
		bool handleInput(float frameTime);
		void handleInputState(float frameTime);
		float restrictCameraCoordinate(float t, float w, float res);
		void startFrame();
		void finishFrame();
		void drawTerrain();
		void drawTexts();
		void drawOverlays();
		void drawRectangle(const Common::Rectangle& r,
				const Common::Color& c, float alpha, bool onlyframes);
		void drawLine(const Common::Vector3& p1, const Common::Vector3& p2, const Common::Color& c);
		const boost::shared_ptr<Common::Texture> soldierTexture(const SoldierPtr p,
				float& xp, float& yp, float& sxp, float& syp, float& scale);
		void drawEntities();
		void setFocusSoldier();
		Common::Vector3 getMousePositionOnField() const;
		void drawSoldierName(const SoldierPtr s, const Common::Color& c);
		float getDrawRadius() const;
		void drawOverlayText(const char* text, float size, const Common::Color& c,
				float x, float y, bool centered, bool pixelcoords = false);

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
		boost::shared_ptr<Common::Texture> mTankTexture[NUM_SIDES][4];
		boost::shared_ptr<Common::Texture> mDestroyedTankTexture[NUM_SIDES][4];
		boost::shared_ptr<Common::Texture> mSoldierTexture[NUM_SIDES][4];
		boost::shared_ptr<Common::Texture> mFallenSoldierTexture[NUM_SIDES];
		boost::shared_ptr<Common::Texture> mSoldierShadowTexture;
		boost::shared_ptr<Common::Texture> mGrassTexture;
		boost::shared_ptr<Common::Texture> mTreeTexture;
		boost::shared_ptr<Common::Texture> mTreeShadowTexture;
		boost::shared_ptr<Common::Texture> mWeaponPickupTextures[int(WeaponPickupTexture::END)];
		Common::TextMap mTextMap;
		SoldierPtr mSoldier;
		bool mSoldierVisible;
		bool mObserver;
		bool mShooting;
		bool mRestarting;
		bool mDriving;
		DebugSymbolCollection mDebugSymbols;
		SoldierRank mSoldierRank;
};

typedef boost::shared_ptr<Driver> DriverPtr;

};

#endif

