#include <cassert>

#include "Event.h"
#include "World.h"
#include "InfoChannel.h"
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
	if(p->driving()) {
		assert(p->getMountPoint());
		p->getSensorySystem()->addSound(p->getMountPoint());
	}
}

WeaponPickupEvent::WeaponPickupEvent(WeaponPtr w)
	: Event(EventType::WeaponPickup),
	mWeapon(w)
{
}

void WeaponPickupEvent::handleEvent(SoldierPtr p)
{
	if(p->hasWeaponType(mWeapon->getName()))
		return;

	p->addWeapon(mWeapon);
}


}

