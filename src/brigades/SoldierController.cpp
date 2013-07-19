#include "SoldierController.h"
#include "Soldier.h"
#include "World.h"
#include "SoldierQuery.h"

#include "SoldierAgent.h"

using namespace Common;

#define SOLDIERCONTROLLER_OBSTACLE_CACHE_UPDATE_TIME 1.0

namespace Brigades {

AttackOrder::AttackOrder(const Common::Vector3& p, const Common::Vector3& d, float width)
	: CenterPoint(p)
{
	Vector3 v = d - p;
	v.normalize();
	v *= width;
	DefenseLineToRight.x = v.y;
	DefenseLineToRight.y = -v.x;
}

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

void SoldierController::moveTo(const Common::Vector3& dir, float time, bool autorotate)
{
	if(mSoldier->sleeping() || mSoldier->eating()) {
		return;
	}

	if(isnan(dir.x) || isnan(dir.y)) {
		std::cout << "moveTo: warning: dir: " << dir << "\n";
		return;
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
}

void SoldierController::turnTo(const Common::Vector3& dir)
{
	if(mSoldier->sleeping() || mSoldier->eating())
		return;

	mSoldier->setXYRotation(atan2(dir.y, dir.x));
}

void SoldierController::turnBy(float rad)
{
	if(mSoldier->sleeping() || mSoldier->eating())
		return;

	mSoldier->addXYRotation(rad);
}

void SoldierController::setVelocityToHeading()
{
	if(mSoldier->sleeping() || mSoldier->eating())
		return;

	mSoldier->setVelocityToHeading();
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

}

