#ifndef BRIGADES_EVENT_H
#define BRIGADES_EVENT_H

#include <boost/shared_ptr.hpp>

namespace Brigades {

enum class EventType {
	Sound,
	WeaponPickup,
};

class Soldier;
class Weapon;
typedef boost::shared_ptr<Soldier> SoldierPtr;
typedef boost::shared_ptr<Weapon> WeaponPtr;

class Event {
	public:
		Event(EventType t);
		virtual ~Event() { }
		EventType getType() const;
		virtual void handleEvent(SoldierPtr p) = 0;

	private:
		EventType mType;
};

class SoundEvent : public Event {
	public:
		SoundEvent(SoldierPtr soundmaker);
		void handleEvent(SoldierPtr p);

	private:
		SoldierPtr mSoundMaker;
};

class WeaponPickupEvent : public Event {
	public:
		WeaponPickupEvent(WeaponPtr w);
		void handleEvent(SoldierPtr p);

	private:
		WeaponPtr mWeapon;
};

typedef boost::shared_ptr<Event> EventPtr;

}

#endif

