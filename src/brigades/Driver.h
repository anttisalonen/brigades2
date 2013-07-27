#ifndef BRIGADES_DRIVER_H
#define BRIGADES_DRIVER_H

#include <array>

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
#include "InfoChannel.h"
#include "PlayerAgent.h"
#include "AgentDirectory.h"
#include "SoldierAction.h"


namespace Brigades {

enum class SpriteType {
	Soldier,
	Tree,
	Bullet,
	WeaponPickup,
	BrightSpot,
	Foxhole,
	Icon
};

struct Sprite {
	Sprite(const Common::Vector3& pos, SpriteType t, float scale,
			boost::shared_ptr<Common::Texture> texture,
			boost::shared_ptr<Common::Texture> shadow, float xp, float yp,
			float sxp, float syp, float alpha = 1.0f)
		: mPosition(pos), mSpriteType(t), mScale(scale), mTexture(texture), mShadowTexture(shadow),
		mXP(xp), mYP(yp), mSXP(sxp), mSYP(syp), mAlpha(alpha) { }
	Common::Vector3 mPosition;
	SpriteType mSpriteType;
	float mScale;
	boost::shared_ptr<Common::Texture> mTexture;
	boost::shared_ptr<Common::Texture> mShadowTexture;
	float mXP;
	float mYP;
	float mSXP;
	float mSYP;
	float mAlpha;
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

struct InfoMessage {
	InfoMessage() { }
	InfoMessage(const Common::Color& c, const char* t)
		: mColor(c),
		mText(t) { }
	Common::Color mColor;
	std::string mText;
};

struct SpeechBubble {
	SpeechBubble(const char* s = "", float ts = 8.0f)
		: mTime(ts), mText(s) { }
	bool expired(float t);

	Common::Countdown mTime;
	std::string mText;
};

enum class MapLevel {
	Normal,
	Tactic
};

enum class MilitaryBranch {
	MechInf,
};

struct UnitIconDescriptor {
	UnitIconDescriptor(MilitaryBranch b, SoldierRank r, bool blues)
		: branch(b), rank(r), blue(blues) { }
	MilitaryBranch branch;
	SoldierRank rank;
	bool blue;

	inline bool operator==(const UnitIconDescriptor& rhs) const;
	inline bool operator!=(const UnitIconDescriptor& rhs) const;
	inline bool operator<(const UnitIconDescriptor& f) const;
};

bool UnitIconDescriptor::operator==(const UnitIconDescriptor& rhs) const
{
	return branch == rhs.branch && rank == rhs.rank && blue == rhs.blue;
}

bool UnitIconDescriptor::operator!=(const UnitIconDescriptor& rhs) const
{
	return !(*this == rhs);
}

bool UnitIconDescriptor::operator<(const UnitIconDescriptor& f) const
{
	if(branch != f.branch)
		return branch < f.branch;
	if(rank != f.rank)
		return rank < f.rank;
	return blue < f.blue;
}

class Driver : public DebugOutput, public InfoChannel {
	public:
		Driver(WorldPtr w, bool observer, SoldierRank r);
		~Driver();
		void init();
		void run();
		void markArea(const Common::Color& c, const Common::Rectangle& r, bool onlyframes);
		void addArrow(const Common::Color& c, const Common::Vector3& start, const Common::Vector3& arrow);
		void addMessage(const SoldierQuery* s, const Common::Color& c, const char* text);
		void say(const SoldierQuery& s, const char* msg);

	private:
		void loadTextures();
		void loadFont();
		Common::Color mapUnitIconColor(bool first, const Common::Color& c);
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
		void drawDefenseLine(const AttackOrder& r, const Common::Color& c, float alpha);
		void drawRectangle(const Common::Rectangle& r,
				const Common::Color& c, float alpha, bool onlyframes);
		void drawLine(const Common::Vector3& p1, const Common::Vector3& p2, const Common::Color& c);
		void drawArrow(const Common::Vector3& start, const Common::Vector3& end, const Common::Color& c);
		const boost::shared_ptr<Common::Texture> soldierTexture(bool soldier, bool dead, float xyrot,
				bool firstSide, float& sxp, float& syp,
				float& xp, float& yp, float& scale);
		const boost::shared_ptr<Common::Texture> getUnitIconTexture(const UnitIconDescriptor& d);
		const boost::shared_ptr<Common::Texture> unitIconTexture(const SoldierQuery& p, float& scale);
		void drawEntities();
		void drawRoads();
		void setFocusSoldier();
		Common::Vector3 getMousePositionOnField() const;
		void updateMousePositionOnField();
		void drawText(const char* text, float size, const Common::Color& c,
				const Common::Vector3& pos, bool centered);
		void drawSoldierName(const SoldierQuery& s, const Common::Color& c);
		void drawSoldierGotoMarker(const SoldierQuery& s, bool alwaysdraw);
		float getDrawRadius() const;
		void drawOverlayText(const char* text, float size, const Common::Color& c,
				float x, float y, bool centered, bool pixelcoords = false);
		Common::Color getGroupRectangleColor(const SoldierQuery& commandee, float brightness = 1.0f);
		int getNumberOfAvailableCommandees(const SoldierQuery& p);
		void includeSoldierSprite(std::set<Sprite>& sprites, const SoldierPtr s, bool addbrightspot = false);
		void includeSoldierSprite(std::set<Sprite>& sprites, const SoldierQuery& s, bool addbrightspot = false);
		void includeArmorSprite(std::set<Sprite>& sprites, const ArmorQuery& s, bool addbrightspot = false);
		void includeUnitIcon(std::set<Sprite>& sprites, const SoldierQuery& s, bool addbrightspot = false);
		bool allCommandeesDefending() const;
		void setLight();
		void updateAgents(float time);
		void applyPendingActions();
		Common::Vector3 terrainPositionToScreenPosition(const Common::Vector3& pos);

		WorldPtr mWorld;
		Common::Clock mClock;
		SDL_Surface* mScreen;
		TTF_Font* mFont;
		bool mPaused;
		float mScaleLevel;
		float mScaleLevelVelocity;
		bool mFreeCamera;
		Common::Vector3 mCamera;
		Common::Vector3 mCameraMouseOffset;
		Common::Vector3 mCameraVelocity;
		boost::shared_ptr<Common::Texture> mTankTexture[NUM_SIDES][4];
		boost::shared_ptr<Common::Texture> mDestroyedTankTexture[NUM_SIDES][4];
		boost::shared_ptr<Common::Texture> mSoldierTexture[NUM_SIDES][4];
		boost::shared_ptr<Common::Texture> mFallenSoldierTexture[NUM_SIDES];
		boost::shared_ptr<Common::Texture> mSoldierShadowTexture;
		boost::shared_ptr<Common::Texture> mGrassTexture;
		boost::shared_ptr<Common::Texture> mTreeTexture;
		boost::shared_ptr<Common::Texture> mFoxholeTexture;
		boost::shared_ptr<Common::Texture> mTreeShadowTexture;
		boost::shared_ptr<Common::Texture> mWeaponPickupTextures[int(WeaponPickupTexture::END)];
		boost::shared_ptr<Common::Texture> mBrightSpot;
		boost::shared_ptr<Common::Texture> mUnitIconShadowTexture;
		boost::shared_ptr<Common::Texture> mRoadTexture;
		Common::TextMap mTextMap;
		SoldierQueryPtr mSoldier;
		SoldierPtr mFocusSoldier;
		bool mObserver;
		bool mRestarting;
		DebugSymbolCollection mDebugSymbols;
		std::array<InfoMessage, 5> mInfoMessages;
		std::map<SoldierQuery, SpeechBubble> mSpeechBubbles;
		Common::Countdown mSpeechBubbleTimer;
		unsigned int mNextInfoMessageIndex;
		SoldierRank mSoldierRank;
		InputState* mInputState;
		boost::shared_ptr<PlayerAgent> mPlayerAgent;

		AttackOrder mDrawnAttackOrder;
		bool mCreatingAttackOrder;
		SoldierQueryPtr mSelectedCommandee;
		float mTimeAcceleration;
		MapLevel mMapLevel;
		std::map<UnitIconDescriptor, boost::shared_ptr<Common::Texture>> mUnitIconTextures;
		AgentDirectory mAgentDirectory;

		Common::Color mLight;
		std::vector<SoldierAction> mPendingActions;
};

typedef boost::shared_ptr<Driver> DriverPtr;

};

#endif

