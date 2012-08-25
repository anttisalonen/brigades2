#include "Armory.h"
#include "World.h"

#include "common/Math.h"
#include "common/Random.h"

using namespace Common;

namespace Brigades {

Weapon::Weapon(const char* name, float range, float velocity, float loadtime,
		float variation,
		float softd,
		float lightd,
		float heavyd)
	: mName(name), mRange(range),
	mVelocity(velocity),
	mLoadTime(loadtime),
	mVariation(variation),
	mSoftDamage(softd),
	mLightArmorDamage(lightd),
	mHeavyArmorDamage(heavyd)
{
}

void Weapon::update(float time)
{
	mLoadTime.doCountdown(time);
	mLoadTime.check();
}

bool Weapon::canShoot() const
{
	return !mLoadTime.running();
}

float Weapon::getRange() const
{
	return mRange;
}

float Weapon::getVelocity() const
{
	return mVelocity;
}

float Weapon::getLoadTime() const
{
	return mLoadTime.getMaxTime();
}

float Weapon::getVariation() const
{
	return mVariation;
}

void Weapon::shoot(WorldPtr w, const SoldierPtr s, const Vector3& dir)
{
	if(!canShoot())
		return;

	Vector3 d = Math::rotate2D(dir.normalized(), Random::clamped() * mVariation);

	w->addBullet(shared_from_this(), s, d);
	mLoadTime.rewind();
}

float Weapon::getDamageAgainstSoftTargets() const
{
	return mSoftDamage;
}

float Weapon::getDamageAgainstLightArmor() const
{
	return mLightArmorDamage;
}

float Weapon::getDamageAgainstHeavyArmor() const
{
	return mHeavyArmorDamage;
}

const char* Weapon::getName() const
{
	return mName;
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


