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

	updateShootTarget();

	move(time);
	tryToShoot();
}

void SoldierController::move(float time)
{
	Vector3 vel;

	Vector3 tot = defaultMovement(time);

	if(mTargetSoldier) {
		vel = mSteering->pursuit(*mTargetSoldier);
	} else {
		if(mWorld->teamWon() < 0) {
			vel = mSteering->wander();
		}
	}
	vel.truncate(10.0f);

	mSteering->accumulate(tot, vel);

	if(!mTargetSoldier && mWorld->teamWon() < 0) {
		std::vector<Entity*> neighbours;
		for(auto n : mSoldier->getSensorySystem()->getSoldiers()) {
			if(n->getSideNum() == mSoldier->getSideNum()) {
				neighbours.push_back(n.get());
			}
		}

		Vector3 sep = mSteering->separation(neighbours);
		sep.truncate(10.0f);
		if(mSteering->accumulate(tot, sep)) {
			Vector3 coh = mSteering->cohesion(neighbours);
			coh.truncate(1.5f);
			mSteering->accumulate(tot, coh);
		}
	}

	moveTo(tot, time, mShootTargetPosition.null());
	if(!mShootTargetPosition.null()) {
		turnTo(mShootTargetPosition);
	}
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

	float dist = mShootTargetPosition.length();
	if(dist < mSoldier->getWeapon()->getRange()) {
		if(mSoldier->getWeapon()->canShoot()) {
			mSoldier->getWeapon()->shoot(mWorld, mSoldier, mShootTargetPosition);
		}
	}
}

void SoldierController::updateShootTarget()
{
	if(!mTargetSoldier) {
		mShootTargetPosition = Vector3();
		return;
	}

	Vector3 pos = mTargetSoldier->getPosition();
	Vector3 vel = mTargetSoldier->getVelocity();

	float time1, time2;

	Vector3 topos = pos - mSoldier->getPosition();

	if(!Math::tps(topos, vel, mSoldier->getWeapon()->getVelocity(), time1, time2)) {
		mShootTargetPosition = topos;
		return;
	}

	float corrtime = time1 > 0.0f && time1 < time2 ? time1 : time2;
	if(corrtime <= 0.0f) {
		mShootTargetPosition = topos;
		return;
	}

	mShootTargetPosition = topos + vel * corrtime;
}

}

}

