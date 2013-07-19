#ifndef BRIGADES_SOLDIER_H
#define BRIGADES_SOLDIER_H

#include <vector>
#include <list>
#include <set>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "common/Vehicle.h"
#include "common/Steering.h"

#include "Side.h"
#include "SoldierController.h"
#include "Armory.h"
#include "Event.h"

namespace Brigades {

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

class World;
class SensorySystem;

class Soldier;
typedef boost::shared_ptr<Soldier> SoldierPtr;


class Soldier : public Common::Vehicle, public boost::enable_shared_from_this<Soldier> {
	public:
		Soldier(boost::shared_ptr<World> w, bool firstside, SoldierRank rank, WarriorType wt = WarriorType::Soldier);
		void init();
		SidePtr getSide() const;
		int getID() const;
		int getSideNum() const;
		void update(float time) override;
		float getFOV() const; // total FOV in radians
		void die();
		bool isDead() const;
		bool isAlive() const;
		void dig(float time);
		void clearWeapons();
		void addWeapon(WeaponPtr w);
		WeaponPtr getCurrentWeapon();
		void switchWeapon(unsigned int index);
		const std::vector<WeaponPtr>& getWeapons() const;
		const boost::shared_ptr<World> getWorld() const;
		Common::Vector3 getUnitPosition() const;
		boost::shared_ptr<World> getWorld();
		boost::shared_ptr<SensorySystem> getSensorySystem();
		std::set<SoldierPtr> getKnownEnemySoldiers() const;
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

		bool sleeping() const;
		bool eating() const;
		void startSleeping();
		void startEating();
		void stopSleeping();
		void stopEating();
		float getFatigueLevel() const; // >1.0 => should sleep
		float getHungerLevel() const;  // >1.0 => should eat

		// orders for the privates
		void setFormationOffset(const Common::Vector3& v);
		void setDefendPosition(const Common::Vector3& v);
		const Common::Vector3& getFormationOffset() const;
		const Common::Vector3& getDefendPosition() const;

		// orders for the group leader and above
		bool defending() const;
		void setDefending();
		bool giveAttackOrder(const AttackOrder& r);
		const AttackOrder& getAttackOrder() const;
		const Common::Vector3& getCenterOfAttackArea() const;

		// messages from the squad leader and above
		bool reportSuccessfulAttack();

		// messages for the platoon leader and above
		bool successfulAttackReported(const AttackOrder& r);

	private:
		void globalMessage(const char* s);
		void handleSleep(float time);
		void handleEating(float time);

		boost::shared_ptr<World> mWorld;
		SidePtr mSide;
		int mID;
		float mFOV;
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
		float mFatigue = 0.0f;
		float mHunger = 0.0f;
		int mFoodPacks = 3;
		float mSleepTime = 0.0f; // time spent sleeping, 0 => awake
		float mEatTime = 0.0f;   // time spent eating, 0 => not eating

		// crew status
		Common::Vector3 mFormationOffset;
		Common::Vector3 mDefendPosition;

		// leader status
		bool mAttacking;
		AttackOrder mAttackOrder;

		std::string mName;
		bool mEnemyContact;
		Common::Countdown mEnemyContactTimer;

		static int getNextID();
		static std::string generateName();
};

}

#endif

