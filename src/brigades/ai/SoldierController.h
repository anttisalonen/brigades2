#ifndef BRIGADES_AI_SOLDIERCONTROLLER_H
#define BRIGADES_AI_SOLDIERCONTROLLER_H

#include "common/Clock.h"

#include "brigades/World.h"

namespace Brigades {

namespace AI {

class SoldierController : public Brigades::SoldierController {
	public:
		SoldierController(WorldPtr w, SoldierPtr p);
		void act(float time);

	protected:
		Common::SteadyTimer mVisionUpdater;
		SoldierPtr mTargetSoldier;

	private:
		void move(float time);
		void updateTargetSoldier();
		void tryToShoot();
};

}

}

#endif

