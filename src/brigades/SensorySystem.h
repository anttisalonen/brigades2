#ifndef BRIGADES_SENSORYSYSTEM_H
#define BRIGADES_SENSORYSYSTEM_H

#include <vector>

#include <boost/shared_ptr.hpp>

#include "common/Clock.h"

#include "World.h"
#include "Armor.h"

namespace Brigades {

class SensorySystem {
	public:
		SensorySystem(SoldierPtr s);
		bool update(float time);
		std::vector<SoldierPtr> getSoldiers() const;
		const std::vector<Foxhole*>& getFoxholes() const;
		std::vector<ArmorPtr> getVehicles() const;
		void addSound(SoldierPtr s);
		void addSound(ArmorPtr p);
		void clear();

	private:
		void updateFOV();

		SoldierPtr mSoldier;
		Common::SteadyTimer mVisionUpdater;
		std::map<SoldierPtr, float> mSoldiers;
		std::map<ArmorPtr, float> mArmors;

		mutable std::vector<Foxhole*> mFoxholes;
		mutable bool mFoxholesUpdated;
};

typedef boost::shared_ptr<SensorySystem> SensorySystemPtr;

}

#endif

