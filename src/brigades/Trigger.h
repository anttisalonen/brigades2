#ifndef BRIGADES_TRIGGER_H
#define BRIGADES_TRIGGER_H

#include <list>

#include <boost/shared_ptr.hpp>

#include "common/Vector3.h"
#include "common/Vehicle.h"

namespace Brigades {

class Soldier;

class TriggerRegion {
	public:
		virtual ~TriggerRegion() { }
		virtual bool isIn(const Common::Vector3& v) = 0;
};

class CircleTriggerRegion : public TriggerRegion {
	public:
		CircleTriggerRegion(const Common::Vector3& p, float r);
		bool isIn(const Common::Vector3& v);

	private:
		Common::Vector3 mCenter;
		float mRadius;
};

class Trigger {
	public:
		virtual ~Trigger() { }
		virtual void tryTrigger(boost::shared_ptr<Soldier> s) = 0;
		virtual bool update(float time) = 0;
};

typedef boost::shared_ptr<Trigger> TriggerPtr;
typedef boost::shared_ptr<Soldier> SoldierPtr;

class SoundTrigger : public Trigger {
	public:
		SoundTrigger(const SoldierPtr soundmaker, float range);
		void tryTrigger(SoldierPtr s);
		bool update(float time);

	private:
		const SoldierPtr mSoundMaker;
		CircleTriggerRegion mRegion;
};

typedef boost::shared_ptr<SoundTrigger> SoundTriggerPtr;

class TriggerSystem {
	public:
		TriggerSystem();
		void add(TriggerPtr t);
		void update(std::vector<SoldierPtr> soldiers, float time);

	private:
		std::list<TriggerPtr> mTriggers;
};

}

#endif

