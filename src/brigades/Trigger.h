#ifndef BRIGADES_TRIGGER_H
#define BRIGADES_TRIGGER_H

#include <list>

#include <boost/shared_ptr.hpp>

#include "common/Clock.h"
#include "common/Vector3.h"
#include "common/Vehicle.h"

namespace Brigades {

class Soldier;
class Weapon;

class TriggerRegion {
	public:
		virtual ~TriggerRegion() { }
		virtual bool isIn(const Common::Vector3& v) = 0;
};

class CircleTriggerRegion : public TriggerRegion {
	public:
		CircleTriggerRegion(const Common::Vector3& p, float r);
		bool isIn(const Common::Vector3& v);
		const Common::Vector3& getCenter() const;

	private:
		Common::Vector3 mCenter;
		float mRadius;
};

class Trigger {
	public:
		virtual ~Trigger() { }
		virtual void tryTrigger(boost::shared_ptr<Soldier> s) = 0;
		virtual bool update(float time) = 0;
		virtual const char* getName() = 0;
		virtual Common::Vector3 getPosition() = 0;
};

typedef boost::shared_ptr<Trigger> TriggerPtr;
typedef boost::shared_ptr<Soldier> SoldierPtr;
typedef boost::shared_ptr<Weapon> WeaponPtr;

class OnetimeTrigger : public Trigger {
	public:
		virtual bool update(float time);
};

class TimedTrigger : public Trigger {
	public:
		TimedTrigger(float time);
		virtual bool update(float time);

	private:
		Common::Countdown mTimer;
};

class SoundTrigger : public OnetimeTrigger {
	public:
		SoundTrigger(const SoldierPtr soundmaker, float range);
		void tryTrigger(SoldierPtr s);
		const char* getName();
		Common::Vector3 getPosition();

	private:
		const SoldierPtr mSoundMaker;
		CircleTriggerRegion mRegion;
};

class WeaponPickupTrigger : public TimedTrigger {
	public:
		WeaponPickupTrigger(WeaponPtr w, const Common::Vector3& pos);
		void tryTrigger(SoldierPtr s);
		bool update(float time);
		const char* getName();
		Common::Vector3 getPosition();

	private:
		WeaponPtr mWeapon;
		const SoldierPtr mSoundMaker;
		CircleTriggerRegion mRegion;
		bool mPickedUp;
		std::string mName;
};

typedef boost::shared_ptr<SoundTrigger> SoundTriggerPtr;
typedef boost::shared_ptr<WeaponPickupTrigger> WeaponPickupTriggerPtr;

class TriggerSystem {
	public:
		TriggerSystem();
		void add(TriggerPtr t);
		void update(const std::vector<SoldierPtr>& soldiers, float time);
		const std::list<TriggerPtr> getTriggers() const;
		void tryOneShotTrigger(Trigger& t, const std::vector<SoldierPtr>& soldiers);

	private:
		std::list<TriggerPtr> mTriggers;
};

}

#endif

