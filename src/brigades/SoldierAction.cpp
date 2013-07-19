#include <stdio.h>

#include "SoldierAction.h"

namespace Brigades {

SoldierAction::SoldierAction(SAType type)
	: mType(type)
{
	switch(mType) {
		case SAType::SetVelocityToHeading:
		case SAType::SetVelocityToNegativeHeading:
			break;
		default:
			fprintf(stderr, "Error: action %d without data\n", (int)mType);
			assert(0);
			break;
	}
}

SoldierAction::SoldierAction(SAType type, int val)
	: mType(type),
	mIntValue(val)
{
	switch(mType) {
		case SAType::SwitchWeapon:
			break;
		default:
			fprintf(stderr, "Error: action %d with integer data\n", (int)mType);
			assert(0);
			break;
	}
}

SoldierAction::SoldierAction(SAType type, const Common::Vector3& vec)
	: mType(type),
	mVec(vec)
{
	switch(mType) {
		case SAType::Turn:
		case SAType::Move:
		case SAType::Shoot:
			break;
		default:
			fprintf(stderr, "Error: action %d with vector data\n", (int)mType);
			assert(0);
			break;
	}
}

SoldierAction::SoldierAction(SAType type, float val)
	: mType(type),
	mVal(val)
{
	switch(mType) {
		case SAType::TurnBy:
			break;
		default:
			fprintf(stderr, "Error: action %d with floating point data\n", (int)mType);
			assert(0);
			break;
	}
}


bool SoldierAction::execute(SoldierPtr s, boost::shared_ptr<SoldierController>& controller, float time)
{
	switch(mType) {
		case SAType::StartDigging:
		case SAType::StopDigging:
			return false;

		case SAType::Turn:
			return controller->turnTo(mVec);

		case SAType::TurnBy:
			return controller->turnBy(mVal);

		case SAType::SetVelocityToHeading:
			return controller->setVelocityToHeading();

		case SAType::SetVelocityToNegativeHeading:
			return controller->setVelocityToNegativeHeading();

		case SAType::Move:
			return controller->moveTo(mVec, time, false);

		case SAType::Shoot:
			if(!s->getCurrentWeapon())
				return false;
			if(!s->getCurrentWeapon()->canShoot())
				return false;
			s->getCurrentWeapon()->shoot(controller->mWorld, s, mVec);
			return true;

		case SAType::SetDefending:
		case SAType::ReportSuccessfulAttack:
			return false;

		case SAType::SwitchWeapon:
			s->switchWeapon(mIntValue);
			return true;

		case SAType::LineFormation:
		case SAType::ColumnFormation:
		case SAType::GiveOrder:
			return false;
	}
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

