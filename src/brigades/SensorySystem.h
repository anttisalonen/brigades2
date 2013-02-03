#ifndef BRIGADES_SENSORYSYSTEM_H
#define BRIGADES_SENSORYSYSTEM_H

#include <vector>

#include <boost/shared_ptr.hpp>

#include "common/Clock.h"

#include "World.h"

namespace Brigades {

class SensorySystem {
	public:
		SensorySystem(SoldierPtr s);
		bool update(float time);
		const std::vector<SoldierPtr>& getSoldiers() const;
		const std::vector<FoxholePtr>& getFoxholes() const;
		void addSound(SoldierPtr s);

	private:
		void updateFOV();

		SoldierPtr mSoldier;
		Common::SteadyTimer mVisionUpdater;
		std::vector<SoldierPtr> mSoldiers;

		mutable std::vector<FoxholePtr> mFoxholes;
		mutable bool mFoxholesUpdated;
};

typedef boost::shared_ptr<SensorySystem> SensorySystemPtr;

}

#endif

