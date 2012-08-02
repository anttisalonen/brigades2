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

SoundTrigger::SoundTrigger(const boost::shared_ptr<Soldier> soundmaker, float range)
	: mSoundMaker(soundmaker),
	mRegion(soundmaker->getPosition(), range)
{
}

void SoundTrigger::tryTrigger(boost::shared_ptr<Soldier> s)
{
	if(mRegion.isIn(s->getPosition())) {
		EventPtr e(new Event(EventType::Sound, mSoundMaker.get()));
		s->addEvent(e);
	}
}

bool SoundTrigger::update(float time)
{
	return false;
}

TriggerSystem::TriggerSystem()
{
}

void TriggerSystem::add(TriggerPtr t)
{
	mTriggers.push_back(t);
}

void TriggerSystem::update(std::vector<SoldierPtr> soldiers, float time)
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


}

