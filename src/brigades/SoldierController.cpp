#include "SoldierController.h"
#include "Soldier.h"
#include "World.h"
#include "SoldierQuery.h"

#include "SoldierAgent.h"

using namespace Common;

#define SOLDIERCONTROLLER_OBSTACLE_CACHE_UPDATE_TIME 1.0

namespace Brigades {

SoldierController::SoldierController(boost::shared_ptr<Soldier> s)
	: mWorld(s->getWorld()),
	mSoldier(s),
	mSteering(boost::shared_ptr<Steering>(new Steering(*s))),
	mLeaderStatusTimer(1.0f),
	mObstacleCacheTimer(SOLDIERCONTROLLER_OBSTACLE_CACHE_UPDATE_TIME),
	mMovementSoundTimer(1.0f)
{
}

void SoldierController::updateObstacleCache()
{
	mObstacleCache.clear();
	// get trees from 1.5 * max distance we can travel between cache updates
	std::vector<Tree*> trees = mWorld->getTreesAt(mSoldier->getPosition(),
			mSoldier->getMaxSpeed() * SOLDIERCONTROLLER_OBSTACLE_CACHE_UPDATE_TIME * 1.5f);
	for(unsigned int i = 0; i < trees.size(); i++)
		mObstacleCache.push_back(trees[i]);
}

void SoldierController::update(float time)
{
	if(mObstacleCacheTimer.check(time)) {
		updateObstacleCache();
	}

	if(mSoldier->driving() != mDriverSteering) {
		mDriverSteering = mSoldier->driving();
		if(mSoldier->driving()) {
			assert(mSoldier->getMountPoint());
			mSteering = boost::shared_ptr<Steering>(new Steering(*mSoldier->getMountPoint()));
		}
		else {
			mSteering = boost::shared_ptr<Steering>(new Steering(*mSoldier));
		}
	}

}

Vector3 SoldierController::defaultMovement() const
{
	std::vector<WallPtr> wallptrs = mWorld->getWallsAt(mSoldier->getPosition(), mSoldier->getVelocity().length());
	std::vector<Wall*> walls(wallptrs.size());
	for(unsigned int i = 0; i < wallptrs.size(); i++)
		walls[i] = wallptrs[i].get();

	Vector3 obs;
	if(!mSoldier->driving())
		obs = mSteering->obstacleAvoidance(mObstacleCache) * 100.0f;

	Vector3 wal = mSteering->wallAvoidance(walls) * 100.0f;

	Vector3 tot;
	mSteering->accumulate(tot, obs);
	mSteering->accumulate(tot, wal);

	return tot;
}

bool SoldierController::moveTo(const Common::Vector3& dir, float time, bool autorotate)
{
	boost::shared_ptr<Vehicle> veh = mSoldier;
	if(mSoldier->driving())
		veh = mSoldier->getMountPoint();

	if(!ableToMove()) {
		return false;
	}

	if(isnan(dir.x) || isnan(dir.y)) {
		std::cout << "moveTo: warning: dir: " << dir << "\n";
		return false;
	}

	if(dir.null() && !veh->getVelocity().null()) {
		assert(!isnan(veh->getVelocity().x));
		assert(!isnan(veh->getVelocity().y));
		veh->setAcceleration(veh->getVelocity() * -10.0f);
	}
	else {
		assert(time);
		veh->setAcceleration(dir * (10.0f / time));
	}
	if(autorotate && veh->getVelocity().length() > 0.3f)
		veh->setAutomaticHeading();

	mMovementSoundTimer.doCountdown(time);
	if(mMovementSoundTimer.checkAndRewind()) {
		mWorld->createMovementSound(mSoldier);
	}

	// world will ensure the soldier and the vehicle are in the
	// same location. Do not change the position here as it will
	// mess up the CSP in world.

	return true;
}

bool SoldierController::turnTo(const Common::Vector3& dir)
{
	if(!ableToMove())
		return false;

	if(mSoldier->driving())
		mSoldier->getMountPoint()->setXYRotation(atan2(dir.y, dir.x));

	mSoldier->setXYRotation(atan2(dir.y, dir.x));

	return true;
}

bool SoldierController::ableToMove() const
{
	return !mSoldier->sleeping() && !mSoldier->eating() && (!mSoldier->mounted() || mSoldier->driving());
}

bool SoldierController::turnBy(float rad)
{
	if(!ableToMove())
		return false;

	if(mSoldier->driving())
		mSoldier->getMountPoint()->addXYRotation(rad);

	mSoldier->addXYRotation(rad);

	return true;
}

bool SoldierController::setVelocityToHeading()
{
	if(!ableToMove())
		return false;

	if(mSoldier->driving())
		mSoldier->getMountPoint()->setVelocityToHeading();

	mSoldier->setVelocityToHeading();

	return true;
}

bool SoldierController::setVelocityToNegativeHeading()
{
	if(!ableToMove())
		return false;

	if(mSoldier->driving())
		mSoldier->getMountPoint()->setVelocityToNegativeHeading();

	mSoldier->setVelocityToNegativeHeading();

	return true;
}

SoldierQuery SoldierController::getControlledSoldier() const
{
	return SoldierQuery(mSoldier);
}

bool SoldierController::handleLeaderCheck(float time)
{
	if(mLeaderStatusTimer.check(time)) {
		return checkLeaderStatus();
	} else {
		return false;
	}
}

bool SoldierController::checkLeaderStatus()
{
	if(mSoldier->sleeping() || mSoldier->eating())
		return false;

	switch(mSoldier->getRank()) {
		case SoldierRank::Private:
			if(mSoldier->getLeader() &&
					mSoldier->seesSoldier(mSoldier->getLeader()) &&
					mSoldier->getLeader()->isDead()) {
				assert(mSoldier->getCommandees().empty());
				mSoldier->setRank(SoldierRank::Sergeant);
				auto deceased = mSoldier->getLeader();
				auto newleader = deceased->getLeader();
				deceased->removeCommandee(mSoldier);

				for(auto c : deceased->getCommandees()) {
					/* TODO: set up a system so that the privates
					 * not seeing the new sergeant are left dangling. */
					mSoldier->addCommandee(c);
				}
				deceased->getCommandees().clear();
				if(newleader) {
					newleader->removeCommandee(deceased);
					newleader->addCommandee(mSoldier);
				} else {
					mSoldier->setLeader(newleader);
				}
				if(deceased->defending()) {
					mSoldier->setDefending();
				} else {
					mSoldier->giveAttackOrder(deceased->getAttackOrder());
				}
				return true;
			}
			break;

		default:
			break;
	}

	return false;
}

Common::Vector3 SoldierController::createMovement(const Common::Vector3& mov) const
{
	Vector3 tot;
	if(!ableToMove())
		return Vector3();

	tot = defaultMovement();
	mSteering->accumulate(tot, mov);
	return tot;
}

void SoldierController::addGotoOrder(boost::shared_ptr<Soldier> from, const Common::Vector3& pos)
{
	SoldierCommunication comm;
	comm.from = from;
	comm.comm = CommunicationType::Order;
	comm.order = OrderType::GotoPosition;
	Vector3* d = new Vector3(pos);
	comm.data = (void*)d;
	mCommunications.push_back(comm);
}

void SoldierController::addMountVehicleOrder(boost::shared_ptr<Soldier> from, const Common::Vector3& pos)
{
	SoldierCommunication comm;
	comm.from = from;
	comm.comm = CommunicationType::Order;
	comm.order = OrderType::MountVehicle;
	Vector3* d = new Vector3(pos);
	comm.data = (void*)d;
	mCommunications.push_back(comm);
}

void SoldierController::addUnmountVehicleOrder(boost::shared_ptr<Soldier> from)
{
	SoldierCommunication comm;
	comm.from = from;
	comm.comm = CommunicationType::Order;
	comm.order = OrderType::UnmountVehicle;
	mCommunications.push_back(comm);
}

void SoldierController::addAcknowledgement(boost::shared_ptr<Soldier> from)
{
	SoldierCommunication comm;
	comm.from = from;
	comm.comm = CommunicationType::Acknowledgement;
	comm.data = nullptr;
	mCommunications.push_back(comm);
}

void SoldierController::addSuccessReport(boost::shared_ptr<Soldier> from)
{
	SoldierCommunication comm;
	comm.from = from;
	comm.comm = CommunicationType::ReportSuccess;
	comm.data = nullptr;
	mCommunications.push_back(comm);
}

void SoldierController::addFailReport(boost::shared_ptr<Soldier> from)
{
	SoldierCommunication comm;
	comm.from = from;
	comm.comm = CommunicationType::ReportFail;
	comm.data = nullptr;
	mCommunications.push_back(comm);
}

std::vector<SoldierCommunication> SoldierController::fetchCommunications()
{
	auto ret = mCommunications;
	mCommunications.clear();
	return ret;
}

}

