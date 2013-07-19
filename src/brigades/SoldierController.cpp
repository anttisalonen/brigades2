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
}

Vector3 SoldierController::defaultMovement() const
{
	if(mSoldier->sleeping() || mSoldier->eating())
		return Vector3();

	std::vector<WallPtr> wallptrs = mWorld->getWallsAt(mSoldier->getPosition(), mSoldier->getVelocity().length());
	std::vector<Wall*> walls(wallptrs.size());
	for(unsigned int i = 0; i < wallptrs.size(); i++)
		walls[i] = wallptrs[i].get();

	Vector3 obs = mSteering->obstacleAvoidance(mObstacleCache) * 100.0f;
	Vector3 wal = mSteering->wallAvoidance(walls) * 100.0f;

	Vector3 tot;
	mSteering->accumulate(tot, wal);
	mSteering->accumulate(tot, obs);

	return tot;
}

bool SoldierController::moveTo(const Common::Vector3& dir, float time, bool autorotate)
{
	if(mSoldier->sleeping() || mSoldier->eating()) {
		return false;
	}

	if(isnan(dir.x) || isnan(dir.y)) {
		std::cout << "moveTo: warning: dir: " << dir << "\n";
		return false;
	}

	if(dir.null() && !mSoldier->getVelocity().null()) {
		assert(!isnan(mSoldier->getVelocity().x));
		assert(!isnan(mSoldier->getVelocity().y));
		mSoldier->setAcceleration(mSoldier->getVelocity() * -10.0f);
	}
	else {
		assert(time);
		mSoldier->setAcceleration(dir * (10.0f / time));
	}
	mSoldier->Vehicle::update(time);
	if(autorotate && mSoldier->getVelocity().length() > 0.3f)
		mSoldier->setAutomaticHeading();

	mMovementSoundTimer.doCountdown(time);
	if(mMovementSoundTimer.checkAndRewind()) {
		mWorld->createMovementSound(mSoldier);
	}

	return true;
}

bool SoldierController::turnTo(const Common::Vector3& dir)
{
	if(mSoldier->sleeping() || mSoldier->eating())
		return false;

	mSoldier->setXYRotation(atan2(dir.y, dir.x));
	return true;
}

bool SoldierController::turnBy(float rad)
{
	if(mSoldier->sleeping() || mSoldier->eating())
		return false;

	mSoldier->addXYRotation(rad);
	return true;
}

bool SoldierController::setVelocityToHeading()
{
	if(mSoldier->sleeping() || mSoldier->eating())
		return false;

	mSoldier->setVelocityToHeading();
	return true;
}

bool SoldierController::setVelocityToNegativeHeading()
{
	if(mSoldier->sleeping() || mSoldier->eating())
		return false;

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

	if(mSoldier->getWarriorType() != WarriorType::Soldier)
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

Common::Vector3 SoldierController::createMovement(bool defmov, const Common::Vector3& mov) const
{
	Vector3 tot;
	if(defmov)
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

