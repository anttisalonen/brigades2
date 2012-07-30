#include <cfloat>

#include "brigades/ai/SoldierController.h"

using namespace Common;

namespace Brigades {

namespace AI {

SoldierController::SoldierController(WorldPtr w, SoldierPtr p)
	: Brigades::SoldierController(w, p),
	mVisionUpdater(0.25f)
{
}

void SoldierController::act(float time)
{
	if(mVisionUpdater.check(time)) {
		updateTargetSoldier();
	}

	move(time);
	tryToShoot();
}

void SoldierController::move(float time)
{
	std::vector<boost::shared_ptr<Tree>> trees = mWorld->getTreesAt(mSoldier->getPosition(), mSoldier->getVelocity().length());
	std::vector<Obstacle*> obstacles(trees.size());
	for(unsigned int i = 0; i < trees.size(); i++)
		obstacles[i] = trees[i].get();

	std::vector<WallPtr> wallptrs = mWorld->getWallsAt(mSoldier->getPosition(), mSoldier->getVelocity().length());
	std::vector<Wall*> walls(wallptrs.size());
	for(unsigned int i = 0; i < wallptrs.size(); i++)
		walls[i] = wallptrs[i].get();

	Vector3 obs = mSteering.obstacleAvoidance(obstacles);
	Vector3 wal = mSteering.wallAvoidance(walls);

	Vector3 vel;
	if(mTargetSoldier) {
		vel = mSteering.pursuit(*mTargetSoldier);
	} else {
		if(mWorld->teamWon() < 0)
			vel = mSteering.wander();
	}
	vel.truncate(10.0f);

	Vector3 tot;
	mSteering.accumulate(tot, wal);
	mSteering.accumulate(tot, obs);
	mSteering.accumulate(tot, vel);

	mSoldier->setAcceleration(tot * (10.0f / time));
	mSoldier->Vehicle::update(time);
	if(mSoldier->getVelocity().length() > 0.3f)
		mSoldier->setAutomaticHeading();
}

void SoldierController::updateTargetSoldier()
{
	auto soldiers = mWorld->getSoldiersInFOV(mSoldier);
	float distToNearest = FLT_MAX;
	SoldierPtr nearest;
	for(auto s : soldiers) {
		if(!s->isDead() && s->getSideNum() != mSoldier->getSideNum()) {
			float thisdist = Entity::distanceBetween(*mSoldier, *s);
			if(thisdist < distToNearest) {
				distToNearest = thisdist;
				nearest = s;
			}
		}
	}

	if(nearest) {
		mTargetSoldier = nearest;
	}
	else {
		mTargetSoldier = SoldierPtr();
	}
}

void SoldierController::tryToShoot()
{
	if(!mTargetSoldier) {
		return;
	}

	float dist = Entity::distanceBetween(*mTargetSoldier, *mSoldier);
	if(dist < mSoldier->getWeapon()->getRange()) {
		if(mSoldier->getWeapon()->canShoot()) {
			mSoldier->getWeapon()->shoot(mWorld, mSoldier, mTargetSoldier->getPosition() - mSoldier->getPosition());
		}
	}
}

}

}

