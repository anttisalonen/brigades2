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

	return actions;
}

}

}

