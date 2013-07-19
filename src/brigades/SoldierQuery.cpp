#include <stdexcept>

#include "SoldierQuery.h"
#include "SensorySystem.h"
#include "World.h"

#define soldier_query_check() do { if(!queryIsValid()) {assert(0); throw std::runtime_error("invalid soldier query"); } } while(0)
#define foxhole_query_check() do { if(!queryIsValid()) {assert(0); throw std::runtime_error("invalid foxhole query"); } } while(0)

namespace Brigades {

FoxholeQuery::FoxholeQuery(const Foxhole* p)
	: mFoxhole(p)
{
}

bool FoxholeQuery::queryIsValid() const
{
	// TODO
	return true;
}

float FoxholeQuery::getDepth() const
{
	foxhole_query_check();
	return mFoxhole->getDepth();
}

const Common::Vector3& FoxholeQuery::getPosition() const
{
	foxhole_query_check();
	return mFoxhole->getPosition();
}

bool FoxholeQuery::operator<(const FoxholeQuery& f) const
{
	return mFoxhole < f.mFoxhole;
}


SoldierQuery::SoldierQuery()
{
}

SoldierQuery::SoldierQuery(const SoldierPtr s)
	: mSoldier(s)
{
}

bool SoldierQuery::queryIsValid() const
{
	// TODO
	return mSoldier != nullptr;
}

int SoldierQuery::getID() const
{
	soldier_query_check();
	return mSoldier->getID();
}

int SoldierQuery::getSideNum() const
{
	soldier_query_check();
	return mSoldier->getSideNum();
}

bool SoldierQuery::isDead() const
{
	soldier_query_check();
	return mSoldier->isDead();
}

bool SoldierQuery::isAlive() const
{
	soldier_query_check();
	return !isDead();
}

bool SoldierQuery::hasCurrentWeapon() const
{
	return mSoldier->getCurrentWeapon() != nullptr;
}

WeaponQuery SoldierQuery::getCurrentWeapon() const
{
	soldier_query_check();
	auto wp = mSoldier->getCurrentWeapon();
	assert(wp);
	return WeaponQuery(wp);
}

std::vector<WeaponQuery> SoldierQuery::getWeapons() const
{
	soldier_query_check();
	std::vector<WeaponQuery> ret;
	auto sps = mSoldier->getWeapons();
	for(auto s : sps)
		ret.push_back(WeaponQuery(s));
	return ret;
}

Common::Vector3 SoldierQuery::getUnitPosition() const
{
	// TODO
	assert(0);
	return Common::Vector3();
}

std::set<SoldierQuery> SoldierQuery::getKnownEnemySoldiers() const
{
	soldier_query_check();
	std::set<SoldierQuery> ret;
	auto sps = mSoldier->getKnownEnemySoldiers();
	for(auto s : sps)
		ret.insert(SoldierQuery(s));
	return ret;
}

SoldierRank SoldierQuery::getRank() const
{
	soldier_query_check();
	return mSoldier->getRank();
}

std::set<SoldierQuery> SoldierQuery::getSensedSoldiers() const
{
	soldier_query_check();
	std::set<SoldierQuery> ret;
	auto sps = mSoldier->getSensorySystem()->getSoldiers();
	for(auto s : sps)
		ret.insert(SoldierQuery(s));
	return ret;
}

std::set<FoxholeQuery> SoldierQuery::getSensedFoxholes() const
{
	soldier_query_check();
	std::set<FoxholeQuery> ret;
	auto sps = mSoldier->getSensorySystem()->getFoxholes();
	for(auto s : sps)
		ret.insert(FoxholeQuery(s));
	return ret;
}

std::set<ArmorQuery> SoldierQuery::getSensedVehicles() const
{
	soldier_query_check();
	std::set<ArmorQuery> ret;
	auto sps = mSoldier->getSensorySystem()->getVehicles();
	for(auto s : sps)
		ret.insert(ArmorQuery(s));
	return ret;
}

ArmorQuery SoldierQuery::getMountPoint() const
{
	soldier_query_check();
	auto wp = mSoldier->getMountPoint();
	assert(wp);
	return ArmorQuery(wp);
}

std::vector<SoldierQuery> SoldierQuery::getCommandees() const
{
	soldier_query_check();
	std::vector<SoldierQuery> ret;
	auto sps = mSoldier->getCommandees();
	for(auto s : sps)
		ret.push_back(SoldierQuery(s));
	return ret;
}

bool SoldierQuery::hasLeader() const
{
	soldier_query_check();
	return mSoldier->getLeader() != nullptr;
}

SoldierQuery SoldierQuery::getLeader() const
{
	soldier_query_check();
	auto l = mSoldier->getLeader();
	assert(l);
	return SoldierQuery(l);
}

bool SoldierQuery::seesSoldier(const SoldierQuery& s) const
{
	soldier_query_check();
	std::vector<SoldierQuery> ret;
	auto sps = mSoldier->getSensorySystem()->getSoldiers();
	for(auto ss : sps) {
		if(ss == s.mSoldier)
			return true;
	}
	return false;
}

bool SoldierQuery::mounted() const
{
	soldier_query_check();
	return mSoldier->mounted();
}

bool SoldierQuery::hasWeaponType(const char* wname) const
{
	soldier_query_check();
	return mSoldier->hasWeaponType(wname);
}

bool SoldierQuery::isDictator() const
{
	soldier_query_check();
	return mSoldier->isDictator();
}

bool SoldierQuery::canCommunicateWith(const SoldierQuery& p) const
{
	soldier_query_check();
	return mSoldier->canCommunicateWith(*p.mSoldier);
}

bool SoldierQuery::hasRadio() const
{
	soldier_query_check();
	return mSoldier->hasRadio();
}

bool SoldierQuery::hasEnemyContact() const
{
	soldier_query_check();
	return mSoldier->hasEnemyContact();
}

const std::string& SoldierQuery::getName() const
{
	soldier_query_check();
	return mSoldier->getName();
}

bool SoldierQuery::sleeping() const
{
	soldier_query_check();
	return mSoldier->sleeping();
}

bool SoldierQuery::eating() const
{
	soldier_query_check();
	return mSoldier->eating();
}

float SoldierQuery::getFatigueLevel() const
{
	soldier_query_check();
	return mSoldier->getFatigueLevel();
}

float SoldierQuery::getHungerLevel() const
{
	soldier_query_check();
	return mSoldier->getHungerLevel();
}

Common::Vector3 SoldierQuery::getFormationOffset() const
{
	soldier_query_check();
	return mSoldier->getFormationOffset();
}

Common::Vector3 SoldierQuery::getDefendPosition() const
{
	soldier_query_check();
	return mSoldier->getDefendPosition();
}

bool SoldierQuery::defending() const
{
	soldier_query_check();
	return mSoldier->defending();
}

AttackOrder SoldierQuery::getAttackOrder() const
{
	soldier_query_check();
	return mSoldier->getAttackOrder();
}

Common::Vector3 SoldierQuery::getCenterOfAttackArea() const
{
	soldier_query_check();
	return mSoldier->getCenterOfAttackArea();
}

Common::Vector3 SoldierQuery::getPosition() const
{
	soldier_query_check();
	return mSoldier->getPosition();
}

Common::Vector3 SoldierQuery::getHeadingVector() const
{
	soldier_query_check();
	return mSoldier->getHeadingVector();
}

Common::Vector3 SoldierQuery::getVelocity() const
{
	soldier_query_check();
	return mSoldier->getVelocity();
}

float SoldierQuery::getXYRotation() const
{
	soldier_query_check();
	return mSoldier->getXYRotation();
}

bool SoldierQuery::operator==(const SoldierQuery& f) const
{
	return mSoldier == f.mSoldier;
}

bool SoldierQuery::operator!=(const SoldierQuery& f) const
{
	return !(*this == f);
}

bool SoldierQuery::operator<(const SoldierQuery& f) const
{
	return mSoldier < f.mSoldier;
}


}

