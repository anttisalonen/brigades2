#ifndef BRIGADES_SOLDIERACTION_H
#define BRIGADES_SOLDIERACTION_H

#include <boost/shared_ptr.hpp>

#include "SoldierQuery.h"
#include "common/Vector3.h"

namespace Brigades {

enum class SAType {
	StartDigging,
	StopDigging,
	Turn,
	TurnBy,
	SetVelocityToHeading,
	SetVelocityToNegativeHeading,
	Move,
	Shoot,
	SetDefending,
	ReportSuccessfulAttack,
	SwitchWeapon,
	LineFormation,
	ColumnFormation,
	GiveOrder,
};

class SoldierAction {
	public:
		SoldierAction(SAType type);
		SoldierAction(SAType type, const Common::Vector3& vec);
		SoldierAction(SAType type, float val);
		bool execute();

		static SoldierAction SoldierDefendCommand(const SoldierQuery& s, const Common::Vector3& pos);
		static SoldierAction SoldierAttackCommand(const SoldierQuery& s, const AttackOrder& pos);
};

}

#endif

