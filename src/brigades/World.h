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

class Weapon : public boost::enable_shared_from_this<Weapon> {
	public:
		Weapon(float range, float velocity, float loadtime,
				float softd = 1.0f,
				float lightd = 0.0f,
				float heavyd = 0.0f);
		virtual ~Weapon() { }
		void update(float time);
		bool canShoot() const;
		float getRange() const;
		float getVelocity() const;
		float getLoadTime() const;
		void shoot(WorldPtr w, const SoldierPtr s, const Common::Vector3& dir);
		float getDamageAgainstSoftTargets() const;
		float getDamageAgainstLightArmor() const;
		float getDamageAgainstHeavyArmor() const;
		virtual const char* getName() const = 0;

	protected:
		float mRange;
		float mVelocity;
		Common::Countdown mLoadTime;
		float mSoftDamage;
		float mLightArmorDamage;
		float mHeavyArmorDamage;
};

class AssaultRifle : public Weapon {
	public:
		AssaultRifle();
		const char* getName() const;
};

class MachineGun : public Weapon {
	public:
		MachineGun();
		const char* getName() const;
};

class Bazooka : public Weapon {
	public:
		Bazooka();
		const char* getName() const;
};

class Pistol : public Weapon {
	public:
		Pistol();
		const char* getName() const;
};

class AutomaticCannon : public Weapon {
	public:
		AutomaticCannon();
		const char* getName() const;
};

typedef boost::shared_ptr<Weapon> WeaponPtr;

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
		virtual bool handleAttackSuccess(const Common::Rectangle& r) = 0;

		Common::Vector3 defaultMovement(float time);
		void moveTo(const Common::Vector3& dir, float time, bool autorotate);
		void turnTo(const Common::Vector3& dir);
		void turnBy(float rad);
		void setVelocityToHeading();
		boost::shared_ptr<Common::Steering> getSteering();

		boost::shared_ptr<World> mWorld;
		boost::shared_ptr<Soldier> mSoldier;
		boost::shared_ptr<Common::Steering> mSteering;
};

typedef boost::shared_ptr<SoldierController> SoldierControllerPtr;

class SensorySystem;

enum class SoldierRank {
	Private,
	Corporal,
	Sergeant,
	Lieutenant,
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
		SoldierControllerPtr getController();
		void die();
		bool isDead() const;
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
		void addCommandee(SoldierPtr s);
		std::list<SoldierPtr>& getCommandees();
		void setLeader(SoldierPtr s);
		SoldierPtr getLeader();

		// orders for the crew
		void setFormationOffset(const Common::Vector3& v);
		const Common::Vector3& getFormationOffset() const;

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

		// orders for the group leader
		bool defending() const;
		void setDefending();
		void giveAttackOrder(const Common::Rectangle& r);
		const Common::Rectangle& getAttackArea() const;

		// messages for the platoon leader
		void reportSuccessfulAttack(const Common::Rectangle& r);

	private:
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
		Common::Vector3 mFormationOffset;
		WarriorType mWarriorType;
		float mHealth;
		bool mDictator;
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

	private:
		SoldierPtr mShooter;
		Common::Countdown mTimer;
		WeaponPtr mWeapon;
};

typedef boost::shared_ptr<Bullet> BulletPtr;

class World : public boost::enable_shared_from_this<World> {

	public:
		World();
		void create();

		// accessors
		std::vector<TreePtr> getTreesAt(const Common::Vector3& v, float radius) const;
		std::vector<SoldierPtr> getSoldiersAt(const Common::Vector3& v, float radius);
		std::list<BulletPtr> getBulletsAt(const Common::Vector3& v, float radius) const;
		float getWidth() const;
		float getHeight() const;
		SidePtr getSide(bool first) const;
		std::vector<WallPtr> getWallsAt(const Common::Vector3& v, float radius) const;
		std::vector<SoldierPtr> getSoldiersInFOV(const SoldierPtr p);
		int teamWon() const; // -1 => no one has won yet, -2 => no teams alive
		int soldiersAlive(int t) const;
		const TriggerSystem& getTriggerSystem() const;
		const Common::Vector3& getHomeBasePosition(bool first) const;
		float getMaxVisibility() const;

		// modifiers
		void update(float time);
		bool addSoldierAction(const SoldierPtr s, const SoldierAction& a);
		void addBullet(const WeaponPtr w, const SoldierPtr s, const Common::Vector3& dir);

	private:
		void setupSides();
		SoldierPtr addSoldier(bool first, SoldierRank rank, WarriorType wt, bool dictator);
		void addTrees();
		void addWalls();
		void checkSoldierPosition(SoldierPtr s);
		void checkForWin();
		void killSoldier(SoldierPtr s);
		void updateTriggerSystem(float time);
		void addPlatoon(int side);
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
		std::vector<WallPtr> mWalls;
		float mVisibility;
		std::list<BulletPtr> mBullets;
		int mTeamWon;
		int mSoldiersAlive[NUM_SIDES];
		Common::SteadyTimer mWinTimer;
		TriggerSystem mTriggerSystem;
		Common::Vector3 mHomeBasePositions[NUM_SIDES];
		int mSquareSide;
};

};

#endif
