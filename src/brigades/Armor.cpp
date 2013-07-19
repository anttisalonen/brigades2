#include "Armor.h"

namespace Brigades {

Armor::Armor(int sidenum)
	: Common::Vehicle(0.5f, 10.0f, 100.0f),
	mID(getNextID()),
	mSide(sidenum)
{
	mRadius = 3.5f;
	mMaxSpeed = 30.0f;
	mMaxAcceleration = 10.0f;
}

int Armor::getSideNum() const
{
	return mSide;
}

int Armor::getNextID()
{
	static int id = 0;
	return ++id;
}

bool Armor::isDestroyed() const
{
	return mDestroyed;
}

void Armor::destroy()
{
	mDestroyed = true;
	mHealth = 0.0f;
}

int Armor::getID() const
{
	return mID;
}

void Armor::reduceHealth(float n)
{
	mHealth -= n;
	if(mHealth <= 0.0f) {
		destroy();
	}
}

float Armor::getHealth() const
{
	return mHealth;
}

float Armor::damageFactorFromWeapon(const WeaponPtr w) const
{
	return w->getDamageAgainstLightArmor();
}

}

