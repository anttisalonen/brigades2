#include <cassert>

#include "Event.h"
#include "World.h"
#include "SensorySystem.h"

namespace Brigades {

Event::Event(EventType t)
	: mType(t)
{
}

EventType Event::getType() const
{
	return mType;
}

SoundEvent::SoundEvent(SoldierPtr soundmaker)
	: Event(EventType::Sound),
	mSoundMaker(soundmaker)
{
}

void SoundEvent::handleEvent(SoldierPtr p)
{
	p->getSensorySystem()->addSound(mSoundMaker);
}

WeaponPickupEvent::WeaponPickupEvent(WeaponPtr w)
	: Event(EventType::WeaponPickup),
	mWeapon(w)
{
}

void WeaponPickupEvent::handleEvent(SoldierPtr p)
{
	// this should've been checked by now
	assert(!p->hasWeaponType(mWeapon->getName()));
	p->addWeapon(mWeapon);
}


}

