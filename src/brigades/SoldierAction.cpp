#include "SoldierAction.h"

namespace Brigades {

SoldierAction::SoldierAction(SAType type)
{
}

SoldierAction::SoldierAction(SAType type, const Common::Vector3& vec)
{
}

SoldierAction::SoldierAction(SAType type, float val)
{
}


bool SoldierAction::execute()
{
	return false;
}

SoldierAction SoldierAction::SoldierDefendCommand(const SoldierQuery& s, const Common::Vector3& pos)
{
	return SoldierAction(SAType::GiveOrder);
}

SoldierAction SoldierAction::SoldierAttackCommand(const SoldierQuery& s, const AttackOrder& pos)
{
	return SoldierAction(SAType::GiveOrder);
}

}

