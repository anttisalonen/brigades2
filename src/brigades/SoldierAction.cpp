#include <stdio.h>

#include "SoldierAction.h"

namespace Brigades {

AgentDirectory* SoldierAction::AgentDir = nullptr;

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

SoldierAction::SoldierAction(const SoldierQuery& s, OrderType order, const Common::Vector3& pos)
	: mType(SAType::Communication),
	mVec(pos),
	mOrder(order),
	mCommunication(CommunicationType::Order),
	mCommandedSoldier(s)
{
}


bool SoldierAction::execute(SoldierPtr s, boost::shared_ptr<SoldierController>& controller, float time)
{
	switch(mType) {
		case SAType::StartDigging:
			return false;

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
			return false;

		case SAType::ColumnFormation:
			return false;

		case SAType::Communication:
			return doCommunication(s, controller);
	}
	return false;
}

bool SoldierAction::doCommunication(SoldierPtr s, boost::shared_ptr<SoldierController>& controller)
{
	switch(mCommunication) {
		case CommunicationType::Order:
			{
				assert(mCommandedSoldier.queryIsValid());
				SoldierQuery me(s);
				if(!me.canCommunicateWith(mCommandedSoldier)) {
					return false;
				}
				auto cs = mCommandedSoldier.mSoldier;
				assert(cs);
				assert(AgentDir);
				auto cntr = AgentDir->getControllerFor(cs);
				assert(cntr);
				switch(mOrder) {
					case OrderType::GotoPosition:
						cntr->addGotoOrder(s, mVec);
						return true;

					default:
						return false;
				}
			}
			return true;

		default:
			return false;
	}
	return false;
}

void SoldierAction::setAgentDirectory(AgentDirectory* dir)
{
	AgentDir = dir;
}

}

