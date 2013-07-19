#ifndef BRIGADES_AI_SOLDIERCONTROLLER_H
#define BRIGADES_AI_SOLDIERCONTROLLER_H

#include <deque>
#include <set>

#include "common/Clock.h"
#include "common/Rectangle.h"

#include "brigades/World.h"
#include "brigades/SoldierAgent.h"

namespace Brigades {

namespace AI {

class SoldierAgent : public Brigades::SoldierAgent {
	public:
		SoldierAgent(boost::shared_ptr<SoldierController> s);
		virtual std::vector<SoldierAction> update(float time) override;
		virtual void newCommunication(const SoldierCommunication& comm) override;

	private:
		std::vector<SoldierAction> mPendingActions;
		Common::Vector3 mMoveTarget;
};

}

}

#endif

