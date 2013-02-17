#include "Trigger.h"
#include "World.h"

using namespace Common;

namespace Brigades {

CircleTriggerRegion::CircleTriggerRegion(const Vector3& p, float r)
	: mCenter(p),
	mRadius(r)
{
}

bool CircleTriggerRegion::isIn(const Common::Vector3& v)
{
	float dist = v.distance2(mCenter);
	return dist <= mRadius * mRadius;
}

const Vector3& CircleTriggerRegion::getCenter() const
{
	return mCenter;
}

bool OnetimeTrigger::update(float time)
{
	return false;
}

TimedTrigger::TimedTrigger(float time)
	: mTimer(time)
{
}

bool TimedTrigger::update(float time)
{
	mTimer.doCountdown(time);
	return mTimer.check();
}

SoundTrigger::SoundTrigger(const boost::shared_ptr<Soldier> soundmaker, float range)
	: mSoundMaker(soundmaker),
	mRegion(soundmaker->getPosition(), range)
{
}

void SoundTrigger::tryTrigger(boost::shared_ptr<Soldier> s)
{
	if(mRegion.isIn(s->getPosition())) {
		s->addEvent(boost::shared_ptr<SoundEvent>(new SoundEvent(mSoundMaker)));
	}
}

const char* SoundTrigger::getName()
{
	return "Sound";
}

Vector3 SoundTrigger::getPosition()
{
	return mRegion.getCenter();
}

WeaponPickupTrigger::WeaponPickupTrigger(WeaponPtr w, const Vector3& pos)
	: TimedTrigger(60.0f),
	mWeapon(w),
	mRegion(pos, 1.0f),
	mPickedUp(false)
{
	mName = std::string("WeaponPickup") + std::string(mWeapon->getName());
}

void WeaponPickupTrigger::tryTrigger(SoldierPtr s)
{
	if(!s->isDead() &&
			s->getWarriorType() == WarriorType::Soldier &&
			mRegion.isIn(s->getPosition()) &&
			!s->hasWeaponType(mWeapon->getName()) &&
			!mPickedUp) {
		s->addEvent(boost::shared_ptr<WeaponPickupEvent>(new WeaponPickupEvent(mWeapon)));
		mPickedUp = true;
	}
}

bool WeaponPickupTrigger::update(float time)
{
	return !mPickedUp && !TimedTrigger::update(time);
}

const char* WeaponPickupTrigger::getName()
{
	return mName.c_str();
}

Vector3 WeaponPickupTrigger::getPosition()
{
	return mRegion.getCenter();
}

TriggerSystem::TriggerSystem()
{
}

void TriggerSystem::add(TriggerPtr t)
{
	mTriggers.push_back(t);
}

void TriggerSystem::update(const std::vector<SoldierPtr>& soldiers, float time)
{
	auto it = mTriggers.begin();
	while(it != mTriggers.end()) {
		for(auto s : soldiers) {
			(*it)->tryTrigger(s);
		}

		if(!(*it)->update(time)) {
			it = mTriggers.erase(it);
		} else {
			++it;
		}
	}

}

void TriggerSystem::tryOneShotTrigger(Trigger& t, const std::vector<SoldierPtr>& soldiers)
{
	for(auto s : soldiers) {
		t.tryTrigger(s);
	}
}

const std::list<TriggerPtr> TriggerSystem::getTriggers() const
{
	return mTriggers;
}

}

