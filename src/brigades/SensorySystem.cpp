#include "SensorySystem.h"

namespace Brigades {

SensorySystem::SensorySystem(SoldierPtr s)
	: mSoldier(s),
	mVisionUpdater(0.25f)
{
}

bool SensorySystem::update(float time)
{
	if(mVisionUpdater.check(time)) {
		updateFOV();
		return true;
	}
	return false;
}

const std::vector<SoldierPtr> SensorySystem::getSoldiers() const
{
	return mSoldiers;
}

void SensorySystem::updateFOV()
{
	mSoldiers = mSoldier->getWorld()->getSoldiersInFOV(mSoldier);
}


}

