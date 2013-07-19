#include <stdexcept>

#include "WeaponQuery.h"

#define weapon_query_check() do { if(!queryIsValid()) {assert(0); throw std::runtime_error("invalid weapon query"); } } while(0)

namespace Brigades {


WeaponQuery::WeaponQuery(const WeaponPtr s)
	: mWeapon(s)
{
}

bool WeaponQuery::queryIsValid() const
{
	// TODO
	return true;
}

WeaponType WeaponQuery::getWeaponType() const
{
	weapon_query_check();
	return *mWeapon->getWeaponType();
}

bool WeaponQuery::canShoot() const
{
	weapon_query_check();
	return mWeapon->canShoot();
}

float WeaponQuery::getRange() const
{
	weapon_query_check();
	return mWeapon->getRange();
}

float WeaponQuery::getVelocity() const
{
	weapon_query_check();
	return mWeapon->getVelocity();
}

float WeaponQuery::getLoadTime() const
{
	weapon_query_check();
	return mWeapon->getLoadTime();
}

float WeaponQuery::getVariation() const
{
	weapon_query_check();
	return mWeapon->getVariation();
}

float WeaponQuery::getDamageAgainstSoftTargets() const
{
	weapon_query_check();
	return mWeapon->getDamageAgainstSoftTargets();
}

float WeaponQuery::getDamageAgainstLightArmor() const
{
	weapon_query_check();
	return mWeapon->getDamageAgainstLightArmor();
}

float WeaponQuery::getDamageAgainstHeavyArmor() const
{
	weapon_query_check();
	return mWeapon->getDamageAgainstHeavyArmor();
}

const char* WeaponQuery::getName() const
{
	weapon_query_check();
	return mWeapon->getName();
}

bool WeaponQuery::speedVariates() const
{
	weapon_query_check();
	return mWeapon->speedVariates();
}

bool WeaponQuery::operator==(const WeaponQuery& f) const
{
	return mWeapon == f.mWeapon;
}

bool WeaponQuery::operator!=(const WeaponQuery& f) const
{
	return !(*this == f);
}


}

