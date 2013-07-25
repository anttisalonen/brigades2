#include <stdio.h>

#include <cassert>
#include <cfloat>
#include <climits>
#include <stdexcept>
#include <queue>
#include <set>

#include "common/Random.h"

#include "brigades/SensorySystem.h"
#include "brigades/DebugOutput.h"
#include "brigades/InfoChannel.h"

#include "brigades/ai/SoldierAgent.h"

#define SECTOR_OWN_PRESENCE	0x01
#define SECTOR_ENEMY_PRESENCE	0x02
#define SECTOR_PLANNED_ATTACK	0x04

using namespace Common;

namespace Brigades {

namespace AI {

SoldierAgent::SoldierAgent(boost::shared_ptr<SoldierController> s)
	: Brigades::SoldierAgent(s)
{
}

std::vector<SoldierAction> SoldierAgent::update(float time)
{
	std::vector<SoldierAction> actions;

	actions.insert(actions.end(), mPendingActions.begin(), mPendingActions.end());
	mPendingActions.clear();

	auto soldier = getControlledSoldier();
	if(!mMoveTarget.null()) {
		Vector3 moveDiff = mMoveTarget - soldier.getPosition();
		if(moveDiff.length2() > 1.0f) {
			Vector3 tot = createMovement(moveDiff);
			actions.push_back(SoldierAction(SAType::Move, tot));
			actions.push_back(SoldierAction(SAType::Turn, moveDiff));
		}

		if(!soldier.mounted()) {
			auto veh = soldier.getSensedVehicles();
			for(auto& v : veh) {
				if(!v.occupied() && soldier.getPosition().distance(v.getPosition()) < 5.0f) {
					actions.push_back(SoldierAction(SAType::Mount));
					break;
				}
			}
		}
	}

	return actions;
}

void SoldierAgent::newCommunication(const SoldierCommunication& comm)
{
	// TODO
	auto soldier = getControlledSoldier();
	assert(comm.from == soldier.getLeader());
	printf("Received communication!\n");
	switch(comm.comm) {
		case CommunicationType::Order:
			switch(comm.order) {
				case OrderType::GotoPosition:
					{
						Vector3* pos = (Vector3*)comm.data;
						mMoveTarget = *pos;
						delete pos;
					}
					break;
			}
			break;
		case CommunicationType::Acknowledgement:
			break;
		case CommunicationType::ReportSuccess:
			break;
		case CommunicationType::ReportFail:
			break;
	}
	return;
}

}

}

