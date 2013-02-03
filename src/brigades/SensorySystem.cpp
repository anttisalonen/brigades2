#include "SensorySystem.h"

namespace Brigades {

SensorySystem::SensorySystem(SoldierPtr s)
	: mSoldier(s),
	mVisionUpdater(0.25f),
	mFoxholesUpdated(false)
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

const std::vector<SoldierPtr>& SensorySystem::getSoldiers() const
{
	return mSoldiers;
}

const std::vector<FoxholePtr>& SensorySystem::getFoxholes() const
{
	if(!mFoxholesUpdated) {
		mFoxholes = mSoldier->getWorld()->getFoxholesInFOV(mSoldier);
		mFoxholesUpdated = true;
	}
	return mFoxholes;
}

void SensorySystem::updateFOV()
{
	mSoldiers = mSoldier->getWorld()->getSoldiersInFOV(mSoldier);
	mFoxholesUpdated = false;
}

void SensorySystem::addSound(SoldierPtr s)
{
	mSoldiers.push_back(s);
}

}

