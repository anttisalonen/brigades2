#ifndef BRIGADES_SOLDIERACTION_H
#define BRIGADES_SOLDIERACTION_H

#include <boost/shared_ptr.hpp>

#include "common/Vector3.h"

#include "Soldier.h"
#include "SoldierQuery.h"
#include "SoldierController.h"

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
		SoldierAction(SAType type, int val);
		SoldierAction(SAType type, float val);
		bool execute(SoldierPtr s, boost::shared_ptr<SoldierController>& controller, float time);

		static SoldierAction SoldierDefendCommand(const SoldierQuery& s, const Common::Vector3& pos);
		static SoldierAction SoldierAttackCommand(const SoldierQuery& s, const AttackOrder& pos);

	private:
		SAType mType;
		Common::Vector3 mVec;
		union {
			float mVal;
			int mIntValue;
		};
		AttackOrder mOrder;
};

}

#endif

