#include "SensorySystem.h"

namespace Brigades {

#define VISION_UPDATE_TIME 0.5
#define RECOLLECTION_TIME 5.0

SensorySystem::SensorySystem(SoldierPtr s)
	: mSoldier(s),
	mVisionUpdater(VISION_UPDATE_TIME),
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

std::vector<SoldierPtr> SensorySystem::getSoldiers() const
{
	std::vector<SoldierPtr> ret;
	for(auto& s : mSoldiers) {
		ret.push_back(s.first);
	}
	return ret;
}

std::vector<ArmorPtr> SensorySystem::getVehicles() const
{
	std::vector<ArmorPtr> ret;
	for(auto& s : mArmors) {
		ret.push_back(s.first);
	}
	return ret;
}

const std::vector<Foxhole*>& SensorySystem::getFoxholes() const
{
	if(!mFoxholesUpdated) {
		mFoxholes = mSoldier->getWorld()->getFoxholesInFOV(mSoldier);
		mFoxholesUpdated = true;
	}
	return mFoxholes;
}

void SensorySystem::updateFOV()
{
	{
		auto currentSoldiers = mSoldier->getWorld()->getSoldiersInFOV(mSoldier);

		// add new soldiers and reset time for previous ones
		for(auto& s : currentSoldiers) {
			mSoldiers[s] = 0.0f;
		}

		for(auto it = mSoldiers.begin(); it != mSoldiers.end(); ) {
			it->second += VISION_UPDATE_TIME;
			if(it->second > RECOLLECTION_TIME) {
				// erase entry
				it = mSoldiers.erase(it);
			} else {
				// continue
				++it;
			}
		}
	}

	{
		auto currentArmors = mSoldier->getWorld()->getArmorsInFOV(mSoldier);

		// add new soldiers and reset time for previous ones
		for(auto& s : currentArmors) {
			mArmors[s] = 0.0f;
		}

		for(auto it = mArmors.begin(); it != mArmors.end(); ) {
			it->second += VISION_UPDATE_TIME;
			if(it->second > RECOLLECTION_TIME) {
				// erase entry
				it = mArmors.erase(it);
			} else {
				// continue
				++it;
			}
		}
	}

	// invalidate foxhole cache
	mFoxholesUpdated = false;
}

void SensorySystem::addSound(SoldierPtr s)
{
	mSoldiers[s] = 0.0f;
}

void SensorySystem::addSound(ArmorPtr s)
{
	mArmors[s] = 0.0f;
}

void SensorySystem::clear()
{
	mSoldiers.clear();
	mArmors.clear();
	mFoxholes.clear();
	mFoxholesUpdated = false;
}


}

