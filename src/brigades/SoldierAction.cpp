#include <stdio.h>

#include "SoldierAction.h"
#include "SensorySystem.h"

namespace Brigades {

AgentDirectory* SoldierAction::AgentDir = nullptr;

SoldierAction::SoldierAction(SAType type)
	: mType(type)
{
	switch(mType) {
		case SAType::SetVelocityToHeading:
		case SAType::SetVelocityToNegativeHeading:
		case SAType::Mount:
		case SAType::Unmount:
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

SoldierAction::SoldierAction(const SoldierQuery& s, OrderType order)
	: mType(SAType::Communication),
	mOrder(order),
	mCommunication(CommunicationType::Order),
	mCommandedSoldier(s)
{
}

SoldierAction::SoldierAction(const SoldierQuery& s, OrderType order, const Common::Vector3& pos)
	: mType(SAType::Communication),
	mVec(pos),
	mOrder(order),
	mCommunication(CommunicationType::Order),
	mCommandedSoldier(s)
{
}

SoldierAction::SoldierAction(const SoldierQuery& s, CommunicationType comm)
	: mType(SAType::Communication),
	mCommunication(comm),
	mCommandedSoldier(s)
{
	assert(comm != CommunicationType::Order);
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

		case SAType::Mount:
			return tryMount(s, controller);

		case SAType::Unmount:
			return tryUnmount(s, controller);
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

					case OrderType::MountVehicle:
						cntr->addMountVehicleOrder(s, mVec);
						return true;

					case OrderType::UnmountVehicle:
						cntr->addUnmountVehicleOrder(s);
						return true;
				}
			}
			return true;

		case CommunicationType::Acknowledgement:
		case CommunicationType::ReportSuccess:
		case CommunicationType::ReportFail:
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
				if(mCommunication == CommunicationType::Acknowledgement) {
					cntr->addAcknowledgement(s);
				} else if(mCommunication == CommunicationType::ReportSuccess) {
					cntr->addSuccessReport(s);
				} else if(mCommunication == CommunicationType::ReportFail) {
					cntr->addFailReport(s);
				} else {
					assert(0);
				}
			}
			return true;
	}
	return false;
}

void SoldierAction::setAgentDirectory(AgentDirectory* dir)
{
	AgentDir = dir;
}

bool SoldierAction::tryMount(SoldierPtr s, boost::shared_ptr<SoldierController>& controller)
{
	auto allseen = s->getSensorySystem()->getVehicles();
	float mindist = 100.0f;
	ArmorPtr found = nullptr;
	for(auto& a : allseen) {
		if(a->driverOccupied() && a->freePassengerSeats() < 1) {
			continue;
		}

		float dist = s->getPosition().distance(a->getPosition());
		if(dist < mindist && dist < 5.0f) {
			found = a;
			mindist = dist;
		}
	}

	if(!found) {
		return false;
	}

	s->mount(found);
	return true;
}

bool SoldierAction::tryUnmount(SoldierPtr s, boost::shared_ptr<SoldierController>& controller)
{
	if(!s->mounted())
		return false;

	s->unmount();

	return true;
}

SAType SoldierAction::getType() const
{
	return mType;
}

}

