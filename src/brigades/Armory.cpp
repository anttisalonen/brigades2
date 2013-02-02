#include "Armory.h"
#include "World.h"

#include "common/Math.h"
#include "common/Random.h"

using namespace Common;

namespace Brigades {

WeaponType::WeaponType(const char* name, float range, float velocity, float loadtime,
		float variation,
		bool speedVariate,
		float softd,
		float lightd,
		float heavyd)
	: mName(name),
	mRange(range),
	mVelocity(velocity),
	mLoadTime(loadtime),
	mVariation(variation),
	mSpeedVariates(speedVariate),
	mSoftDamage(softd),
	mLightArmorDamage(lightd),
	mHeavyArmorDamage(heavyd)
{
}

float WeaponType::getRange() const
{
	return mRange;
}

float WeaponType::getVelocity() const
{
	return mVelocity;
}

float WeaponType::getLoadTime() const
{
	return mLoadTime;
}

float WeaponType::getVariation() const
{
	return mVariation;
}

float WeaponType::getDamageAgainstSoftTargets() const
{
	return mSoftDamage;
}

float WeaponType::getDamageAgainstLightArmor() const
{
	return mLightArmorDamage;
}

float WeaponType::getDamageAgainstHeavyArmor() const
{
	return mHeavyArmorDamage;
}

const char* WeaponType::getName() const
{
	return mName;
}

bool WeaponType::speedVariates() const
{
	return mSpeedVariates;
}


Weapon::Weapon(boost::shared_ptr<WeaponType> type)
	: mWeapon(type),
	mLoading(type->getLoadTime())
{
}

boost::shared_ptr<WeaponType> Weapon::getWeaponType() const
{
	return mWeapon;
}

void Weapon::update(float time)
{
	mLoading.doCountdown(time);
	mLoading.check();
}

bool Weapon::canShoot() const
{
	return !mLoading.running();
}

void Weapon::shoot(WorldPtr w, const SoldierPtr s, const Vector3& dir)
{
	if(!canShoot())
		return;

	float var = mWeapon->getVariation();

	if(mWeapon->speedVariates()) {
		if(s->getSpeed() > 0.5f) {
			var *= 4.0f;
		}
		if(s->getSpeed() > 5.0f) {
			var *= 4.0f;
		}
	}

	var = std::min(var, 0.78539f);

	Vector3 d = Math::rotate2D(dir.normalized(), Random::clamped() * var);

	w->addBullet(shared_from_this(), s, d);
	mLoading.rewind();
}

float Weapon::getRange() const
{
	return mWeapon->getRange();
}

float Weapon::getVelocity() const
{
	return mWeapon->getVelocity();
}

float Weapon::getLoadTime() const
{
	return mWeapon->getLoadTime();
}

float Weapon::getVariation() const
{
	return mWeapon->getVariation();
}

float Weapon::getDamageAgainstSoftTargets() const
{
	return mWeapon->getDamageAgainstSoftTargets();
}

float Weapon::getDamageAgainstLightArmor() const
{
	return mWeapon->getDamageAgainstLightArmor();
}

float Weapon::getDamageAgainstHeavyArmor() const
{
	return mWeapon->getDamageAgainstHeavyArmor();
}

const char* Weapon::getName() const
{
	return mWeapon->getName();
}

bool Weapon::speedVariates() const
{
	return mWeapon->speedVariates();
}


WeaponPtr Armory::getAssaultRifle()
{
	return WeaponPtr(new Weapon(mAssaultRifle));
}

WeaponPtr Armory::getMachineGun()
{
	return WeaponPtr(new Weapon(mMachineGun));
}

WeaponPtr Armory::getBazooka()
{
	return WeaponPtr(new Weapon(mBazooka));
}

WeaponPtr Armory::getPistol()
{
	return WeaponPtr(new Weapon(mPistol));
}

WeaponPtr Armory::getAutomaticCannon()
{
	return WeaponPtr(new Weapon(mAutoCannon));
}

Armory* Armory::getInstance()
{
	assert(mInstance);
	return mInstance;
}

void Armory::setInstance(Armory* a)
{
	mInstance = a;
}

}


