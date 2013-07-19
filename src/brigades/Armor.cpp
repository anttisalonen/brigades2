#include <stdexcept>

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
	mRotation = 0.1f;
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
	return mHealth <= 0.0f;
}

void Armor::destroy()
{
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


#define armor_query_check() do { if(!queryIsValid()) {assert(0); throw std::runtime_error("invalid armor query"); } } while(0)

ArmorQuery::ArmorQuery(const ArmorPtr p)
	: mArmor(p)
{
}

bool ArmorQuery::queryIsValid() const
{
	// TODO
	return mArmor != nullptr;
}

const Common::Vector3& ArmorQuery::getPosition() const
{
	armor_query_check();
	return mArmor->getPosition();
}

int ArmorQuery::getSideNum() const
{
	armor_query_check();
	return mArmor->getSideNum();
}

bool ArmorQuery::isDestroyed() const
{
	armor_query_check();
	return mArmor->isDestroyed();
}

float ArmorQuery::getHealth() const
{
	armor_query_check();
	return mArmor->getHealth();
}

float ArmorQuery::damageFactorFromWeapon(const WeaponPtr w) const
{
	armor_query_check();
	return mArmor->damageFactorFromWeapon(w);
}

float ArmorQuery::getXYRotation() const
{
	armor_query_check();
	return mArmor->getXYRotation();
}

Common::Vector3 ArmorQuery::getHeadingVector() const
{
	armor_query_check();
	return mArmor->getHeadingVector();
}

Common::Vector3 ArmorQuery::getVelocity() const
{
	armor_query_check();
	return mArmor->getVelocity();
}


bool ArmorQuery::operator<(const ArmorQuery& f) const
{
	return mArmor < f.mArmor;
}

}

