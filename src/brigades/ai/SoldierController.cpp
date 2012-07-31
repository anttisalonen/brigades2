#include <cfloat>

#include "brigades/SensorySystem.h"

#include "brigades/ai/SoldierController.h"

using namespace Common;

namespace Brigades {

namespace AI {

SoldierController::SoldierController(SoldierPtr p)
	: Brigades::SoldierController(p)
{
}

void SoldierController::act(float time)
{
	if(mSoldier->getSensorySystem()->update(time)) {
		updateTargetSoldier();
	}

	move(time);
	tryToShoot();
}

void SoldierController::move(float time)
{
	Vector3 vel;

	if(mTargetSoldier) {
		vel = mSteering->pursuit(*mTargetSoldier);
	} else {
		if(mWorld->teamWon() < 0)
			vel = mSteering->wander();
	}
	vel.truncate(10.0f);

	Vector3 tot = defaultMovement(time);
	mSteering->accumulate(tot, vel);

	moveTo(tot, time, true);
}

void SoldierController::updateTargetSoldier()
{
	auto soldiers = mSoldier->getSensorySystem()->getSoldiers();
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

