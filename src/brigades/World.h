#ifndef BRIGADES_WORLD_H
#define BRIGADES_WORLD_H

#include <vector>
#include <map>
#include <list>
#include <set>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "common/Clock.h"
#include "common/Rectangle.h"
#include "common/Vehicle.h"
#include "common/Steering.h"
#include "common/QuadTree.h"
#include "common/CellSpacePartition.h"

#include "Soldier.h"
#include "SoldierController.h"
#include "Side.h"
#include "Armory.h"
#include "Trigger.h"

#define NUM_SIDES 2

namespace Brigades {

class World;

class Tree : public Common::Obstacle {
	public:
		Tree(const Common::Vector3& pos, float radius);
};

typedef boost::shared_ptr<Tree> TreePtr;
typedef boost::shared_ptr<Soldier> SoldierPtr;
typedef boost::shared_ptr<World> WorldPtr;

class SoldierAction {
	public:
		SoldierAction(SoldierPtr p) : mSoldier(p) { }
		virtual ~SoldierAction() { }
		virtual void apply() = 0;

	protected:
		SoldierPtr mSoldier;
};

typedef boost::shared_ptr<SoldierAction> SoldierActionPtr;

typedef boost::shared_ptr<Common::Wall> WallPtr;

class Bullet : public Common::Entity {
	public:
		Bullet(const SoldierPtr shooter, const Common::Vector3& pos, const Common::Vector3& vel, float timeleft);
		void update(float time) override;
		bool isAlive() const;
		SoldierPtr getShooter() const;
		const WeaponPtr getWeapon() const;
		float getOriginalSpeed() const;
		float getFlyTime() const;
		const std::vector<Tree*>& getObstacleCache() const;

	private:
		SoldierPtr mShooter;
		Common::Countdown mTimer;
		WeaponPtr mWeapon;
		std::vector<Tree*> mObstacleCache;
};

typedef boost::shared_ptr<Bullet> BulletPtr;

enum class UnitSize {
	Squad,
	Platoon,
	Company
};

class Foxhole {
	public:
		Foxhole(const WorldPtr world, const Common::Vector3& pos);
		void deepen(float d);
		float getDepth() const;
		const Common::Vector3& getPosition() const;

	private:
		WorldPtr mWorld;
		Common::Vector3 mPosition;
		float mDepth;
};

typedef boost::shared_ptr<Foxhole> FoxholePtr;

struct Timestamp {
	unsigned int Day = 1;
	unsigned int Hour = 0;
	unsigned int Minute = 0;
	unsigned int Second = 0;
	unsigned int Millisecond = 0;

	int secondDifferenceTo(const Timestamp& ts) const;
	void addMilliseconds(unsigned int ms);
};

class World : public boost::enable_shared_from_this<World> {

	public:
		World(float width, float height, float visibility,
				float sounddistance, UnitSize unitsize, bool dictator, Armory& armory);
		void create();

		// accessors
		std::vector<Tree*> getTreesAt(const Common::Vector3& v, float radius) const;
		std::vector<SoldierPtr> getSoldiersAt(const Common::Vector3& v, float radius);
		std::list<BulletPtr> getBulletsAt(const Common::Vector3& v, float radius) const;
		std::vector<Foxhole*> getFoxholesAt(const Common::Vector3& v, float radius) const;
		Foxhole* getFoxholeAt(const Common::Vector3& pos);
		float getWidth() const;
		float getHeight() const;
		SidePtr getSide(bool first) const;
		std::vector<WallPtr> getWallsAt(const Common::Vector3& v, float radius) const;
		std::vector<SoldierPtr> getSoldiersInFOV(const SoldierPtr p);
		std::vector<Foxhole*> getFoxholesInFOV(const SoldierPtr p);
		int teamWon() const; // -1 => no one has won yet, -2 => no teams alive
		int soldiersAlive(int t) const;
		const TriggerSystem& getTriggerSystem() const;
		const Common::Vector3& getHomeBasePosition(bool first) const;
		float getVisibility() const;
		float getVisibilityFactor() const;
		float getShootSoundHearingDistance() const;
		Armory& getArmory() const;
		Common::Rectangle getArea() const;
		const Timestamp& getCurrentTime() const;
		std::string getCurrentTimeAsString() const;

		// modifiers
		void update(float time);
		bool addSoldierAction(const SoldierPtr s, const SoldierAction& a);
		void addBullet(const WeaponPtr w, const SoldierPtr s, const Common::Vector3& dir);
		void dig(float time, const Common::Vector3& pos);

	private:
		void setupSides();
		SoldierPtr addUnit(UnitSize u, unsigned int side, bool reuseLeader = false);
		SoldierPtr addSoldier(bool first, SoldierRank rank, WarriorType wt, bool dictator);
		void addTrees();
		void addWalls();
		void checkSoldierPosition(SoldierPtr s);
		void checkForWin();
		void killSoldier(SoldierPtr s);
		void updateTriggerSystem(float time);
		void updateVisibility();
		SoldierPtr addCompany(int side, bool reuseLeader);
		SoldierPtr addPlatoon(int side, bool reuseLeader);
		SoldierPtr addSquad(int side, bool reuseLeader);
		void addDictator(int side);
		void setHomeBasePositions();
		void reapDeadSoldiers();

		float mWidth;
		float mHeight;
		const unsigned int mMaxSoldiers;
		SidePtr mSides[NUM_SIDES];
		Common::CellSpacePartition<SoldierPtr> mSoldierCSP;
		std::map<int, SoldierPtr> mSoldierMap;
		Common::QuadTree<Tree*> mTrees;
		Common::QuadTree<Foxhole*> mFoxholes;
		std::vector<WallPtr> mWalls;
		float mVisibility;
		float mMaxVisibility;
		float mSoundDistance;
		std::list<BulletPtr> mBullets;
		int mTeamWon;
		int mSoldiersAlive[NUM_SIDES];
		int mSoldiersAtStart;
		SoldierPtr mRootLeader[NUM_SIDES];
		Common::SteadyTimer mWinTimer;
		Common::SteadyTimer mReapTimer;
		TriggerSystem mTriggerSystem;
		Common::Vector3 mHomeBasePositions[NUM_SIDES];
		int mSquareSide;
		Armory& mArmory;
		UnitSize mUnitSize;
		bool mDictator;

		Timestamp mTime;
		Common::Countdown mReinforcementTimer[NUM_SIDES];

		static const float TimeCoefficient;
};

}

#endif
