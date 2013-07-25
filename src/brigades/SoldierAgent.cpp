#include "SoldierAgent.h"

namespace Brigades {

SoldierAgent::SoldierAgent(const boost::shared_ptr<SoldierController> s)
	: mController(s)
{
}

SoldierQuery SoldierAgent::getControlledSoldier() const
{
	return mController->getControlledSoldier();
}

Common::Vector3 SoldierAgent::createMovement(const Common::Vector3& mov) const
{
	return mController->createMovement(mov);
}


}

