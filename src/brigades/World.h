#ifndef BRIGADES_WORLD_H
#define BRIGADES_WORLD_H

#include <vector>
#include <map>
#include <list>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "common/Clock.h"
#include "common/Rectangle.h"
#include "common/Vehicle.h"
#include "common/Steering.h"
#include "common/QuadTree.h"
#include "common/CellSpacePartition.h"

#include "Armory.h"
#include "Event.h"
#include "Trigger.h"

#define NUM_SIDES 2

namespace Brigades {

class World;
class Soldier;

class Tree : public Common::Obstacle {
	public:
		Tree(const Common::Vector3& pos, float radius);
};

typedef boost::shared_ptr<Tree> TreePtr;
typedef boost::shared_ptr<Soldier> SoldierPtr;
typedef boost::shared_ptr<World> WorldPtr;

class Side {
	public:
		Side(bool first);
		bool isFirst() const;
		int getSideNum() const;

	private:
		bool mFirst;
};

typedef boost::shared_ptr<Side> SidePtr;

class Soldier;

class SoldierController : public boost::enable_shared_from_this<SoldierController> {
	public:
		SoldierController();
		SoldierController(boost::shared_ptr<Soldier> s);
		virtual ~SoldierController() { }
		void setSoldier(boost::shared_ptr<Soldier> s);
		virtual void act(float time) = 0;
		virtual bool handleAttackOrder(const Common::Rectangle& r) = 0;
		virtual bool handleAttackSuccess(SoldierPtr s, const Common::Rectangle& r) = 0;

		Common::Vector3 defaultMovement(float time);
		void moveTo(const Common::Vector3& dir, float time, bool autorotate);
		void turnTo(const Common::Vector3& dir);
		void turnBy(float rad);
		void setVelocityToHeading();
		boost::shared_ptr<Common::Steering> getSteering();
		bool handleLeaderCheck(float time);

		virtual void say(boost::shared_ptr<Soldier> s, const char* msg) { }

	protected:
		boost::shared_ptr<World> mWorld;
		boost::shared_ptr<Soldier> mSoldier;
		boost::shared_ptr<Common::Steering> mSteering;
		Common::SteadyTimer mLeaderStatusTimer;

	private:
		bool checkLeaderStatus();
};

typedef boost::shared_ptr<SoldierController> SoldierControllerPtr;

class SensorySystem;

enum class SoldierRank {
	Private,
	Sergeant,
	Lieutenant,
	Captain,
};

enum class WarriorType {
	Soldier,
	Vehicle
};

class Soldier : public Common::Vehicle, public boost::enable_shared_from_this<Soldier> {
	public:
		Soldier(boost::shared_ptr<World> w, bool firstside, SoldierRank rank, WarriorType wt = WarriorType::Soldier);
		void init();
		SidePtr getSide() const;
		int getID() const;
		int getSideNum() const;
		void update(float time) override;
		float getFOV() const; // total FOV in radians
		void setController(SoldierControllerPtr p);
		void setAIController();
		SoldierControllerPtr getController();
		void die();
		bool isDead() const;
		void dig(float time);
		void clearWeapons();
		void addWeapon(WeaponPtr w);
		WeaponPtr getCurrentWeapon();
		void switchWeapon(unsigned int index);
		const std::vector<WeaponPtr>& getWeapons() const;
		const WorldPtr getWorld() const;
		WorldPtr getWorld();
		boost::shared_ptr<SensorySystem> getSensorySystem();
		void addEvent(EventPtr e);
		std::vector<EventPtr>& getEvents();
		bool handleEvents();
		SoldierRank getRank() const;
		void setRank(SoldierRank r);
		void addCommandee(SoldierPtr s);
		void removeCommandee(SoldierPtr s);
		std::list<SoldierPtr>& getCommandees();
		void setLeader(SoldierPtr s);
		SoldierPtr getLeader();
		bool seesSoldier(const SoldierPtr s);

		void setLineFormation(float dist);
		void setColumnFormation(float dist);
		void pruneCommandees();
		WarriorType getWarriorType() const;
		void reduceHealth(float n);
		float getHealth() const;
		float damageFactorFromWeapon(const WeaponPtr w) const;
		bool hasWeaponType(const char* wname) const;
		void setDictator(bool d);
		bool isDictator() const;
		bool canCommunicateWith(const SoldierPtr p) const;
		bool hasRadio() const;
		bool hasEnemyContact() const;
		const std::string& getName() const;
		static const char* rankToString(SoldierRank r);

		// orders for the privates
		void setFormationOffset(const Common::Vector3& v);
		void setDefendPosition(const Common::Vector3& v);
		const Common::Vector3& getFormationOffset() const;
		const Common::Vector3& getDefendPosition() const;

		// orders for the group leader and above
		bool defending() const;
		void setDefending();
		bool giveAttackOrder(const Common::Rectangle& r);
		const Common::Rectangle& getAttackArea() const;

		// messages from the squad leader and above
		bool reportSuccessfulAttack();

		// messages for the platoon leader and above
		bool successfulAttackReported(const Common::Rectangle& r);

	private:
		void globalMessage(const char* s);

		boost::shared_ptr<World> mWorld;
		SidePtr mSide;
		int mID;
		float mFOV;
		SoldierControllerPtr mController;
		bool mAlive;
		std::vector<WeaponPtr> mWeapons;
		unsigned int mCurrentWeaponIndex;
		boost::shared_ptr<SensorySystem> mSensorySystem;
		std::vector<EventPtr> mEvents;
		SoldierRank mRank;
		std::list<SoldierPtr> mCommandees;
		SoldierPtr mLeader;
		WarriorType mWarriorType;
		float mHealth;
		bool mDictator;

		// crew status
		Common::Vector3 mFormationOffset;
		Common::Vector3 mDefendPosition;

		// leader status
		bool mAttacking;
		Common::Rectangle mAttackArea;

		std::string mName;

		static int getNextID();
		static std::string generateName();
};

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

	private:
		SoldierPtr mShooter;
		Common::Countdown mTimer;
		WeaponPtr mWeapon;
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
		const Timestamp& getCurrentTime();

		// accessors
		std::vector<TreePtr> getTreesAt(const Common::Vector3& v, float radius) const;
		std::vector<SoldierPtr> getSoldiersAt(const Common::Vector3& v, float radius);
		std::list<BulletPtr> getBulletsAt(const Common::Vector3& v, float radius) const;
		std::vector<FoxholePtr> getFoxholesAt(const Common::Vector3& v, float radius) const;
		FoxholePtr getFoxholeAt(const Common::Vector3& pos);
		float getWidth() const;
		float getHeight() const;
		SidePtr getSide(bool first) const;
		std::vector<WallPtr> getWallsAt(const Common::Vector3& v, float radius) const;
		std::vector<SoldierPtr> getSoldiersInFOV(const SoldierPtr p);
		std::vector<FoxholePtr> getFoxholesInFOV(const SoldierPtr p);
		int teamWon() const; // -1 => no one has won yet, -2 => no teams alive
		int soldiersAlive(int t) const;
		const TriggerSystem& getTriggerSystem() const;
		const Common::Vector3& getHomeBasePosition(bool first) const;
		float getVisibilityFactor() const;
		float getShootSoundHearingDistance() const;
		Armory& getArmory() const;
		Common::Rectangle getArea() const;

		// modifiers
		void update(float time);
		bool addSoldierAction(const SoldierPtr s, const SoldierAction& a);
		void addBullet(const WeaponPtr w, const SoldierPtr s, const Common::Vector3& dir);
		void dig(float time, const Common::Vector3& pos);

	private:
		void setupSides();
		SoldierPtr addSoldier(bool first, SoldierRank rank, WarriorType wt, bool dictator);
		void addTrees();
		void addWalls();
		void checkSoldierPosition(SoldierPtr s);
		void checkForWin();
		void killSoldier(SoldierPtr s);
		void updateTriggerSystem(float time);
		SoldierPtr addCompany(int side);
		SoldierPtr addPlatoon(int side);
		SoldierPtr addSquad(int side);
		void addDictator(int side);
		void setHomeBasePositions();

		float mWidth;
		float mHeight;
		const unsigned int mMaxSoldiers;
		SidePtr mSides[NUM_SIDES];
		Common::CellSpacePartition<SoldierPtr> mSoldierCSP;
		std::map<int, SoldierPtr> mSoldierMap;
		Common::QuadTree<TreePtr> mTrees;
		Common::QuadTree<FoxholePtr> mFoxholes;
		std::vector<WallPtr> mWalls;
		float mVisibilityFactor;
		float mSoundDistance;
		std::list<BulletPtr> mBullets;
		int mTeamWon;
		int mSoldiersAlive[NUM_SIDES];
		Common::SteadyTimer mWinTimer;
		TriggerSystem mTriggerSystem;
		Common::Vector3 mHomeBasePositions[NUM_SIDES];
		int mSquareSide;
		Armory& mArmory;
		UnitSize mUnitSize;
		bool mDictator;

		Timestamp mTime;
};

};

#endif
