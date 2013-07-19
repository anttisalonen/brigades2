#ifndef BRIGADES_SOLDIERACTION_H
#define BRIGADES_SOLDIERACTION_H

#include <boost/shared_ptr.hpp>

#include "common/Vector3.h"

#include "Soldier.h"
#include "SoldierQuery.h"
#include "SoldierController.h"
#include "AgentDirectory.h"

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
	Communication,
	Mount,
	Unmount,
};

class SoldierAction {
	public:
		SoldierAction(SAType type);
		SoldierAction(SAType type, const Common::Vector3& vec);
		SoldierAction(SAType type, int val);
		SoldierAction(SAType type, float val);
		SoldierAction(const SoldierQuery& s, OrderType order, const Common::Vector3& pos);
		bool execute(SoldierPtr s, boost::shared_ptr<SoldierController>& controller, float time);
		SAType getType() const;

		static void setAgentDirectory(AgentDirectory* dir);


	private:
		bool doCommunication(SoldierPtr s, boost::shared_ptr<SoldierController>& controller);
		bool tryMount(SoldierPtr s, boost::shared_ptr<SoldierController>& controller);
		bool tryUnmount(SoldierPtr s, boost::shared_ptr<SoldierController>& controller);

		SAType mType;
		Common::Vector3 mVec;
		union {
			float mVal;
			int mIntValue;
		};
		AttackOrder mAttackOrder;
		OrderType mOrder;
		CommunicationType mCommunication;
		SoldierQuery mCommandedSoldier;

		static AgentDirectory* AgentDir;
};

}

#endif

