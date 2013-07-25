#ifndef BRIGADES_SOLDIERQUERY_H
#define BRIGADES_SOLDIERQUERY_H

#include "WeaponQuery.h"
#include "Soldier.h"

namespace Brigades {

class SoldierQuery;
class Foxhole;
typedef boost::shared_ptr<SoldierQuery> SoldierQueryPtr;

class FoxholeQuery {
	public:
		FoxholeQuery(const Foxhole* p);
		bool queryIsValid() const;
		float getDepth() const;
		const Common::Vector3& getPosition() const;

		bool operator<(const FoxholeQuery& f) const;

	private:
		const Foxhole* mFoxhole;
		friend FoxholeQuery;
};

class SoldierQuery {
	public:
		SoldierQuery();
		SoldierQuery(const boost::shared_ptr<Soldier> s);
		bool queryIsValid() const;
		int getID() const;
		int getSideNum() const;
		bool isDead() const;
		bool isAlive() const;
		bool hasCurrentWeapon() const;
		WeaponQuery getCurrentWeapon() const;
		std::vector<WeaponQuery> getWeapons() const;
		Common::Vector3 getUnitPosition() const;
		std::set<SoldierQuery> getKnownEnemySoldiers() const;
		SoldierRank getRank() const;
		std::vector<SoldierQuery> getCommandees() const;
		bool hasLeader() const;
		SoldierQuery getLeader() const;
		bool seesSoldier(const SoldierQuery& s) const;
		std::set<SoldierQuery> getSensedSoldiers() const;
		std::set<FoxholeQuery> getSensedFoxholes() const;
		std::set<ArmorQuery> getSensedVehicles() const;
		ArmorQuery getMountPoint() const;
		bool driving() const;

		bool hasWeaponType(const char* wname) const;
		bool isDictator() const;
		bool canCommunicateWith(const SoldierQuery& p) const;
		bool hasRadio() const;
		bool hasEnemyContact() const;
		const std::string& getName() const;

		bool sleeping() const;
		bool eating() const;
		float getFatigueLevel() const; // >1.0 => should sleep
		float getHungerLevel() const;  // >1.0 => should eat
		bool mounted() const;

		Common::Vector3 getFormationOffset() const;
		Common::Vector3 getDefendPosition() const;

		bool defending() const;
		AttackOrder getAttackOrder() const;
		Common::Vector3 getCenterOfAttackArea() const;

		Common::Vector3 getPosition() const;
		Common::Vector3 getHeadingVector() const;
		Common::Vector3 getVelocity() const;
		float getXYRotation() const;

		bool operator==(const SoldierQuery& f) const;
		bool operator!=(const SoldierQuery& f) const;
		bool operator<(const SoldierQuery& f) const;

	private:
		boost::shared_ptr<Soldier> mSoldier;
		friend SoldierQuery;
		friend class SoldierAction;
};

};

#endif

