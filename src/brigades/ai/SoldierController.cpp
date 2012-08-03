#include <cfloat>

#include "brigades/SensorySystem.h"

#include "brigades/ai/SoldierController.h"

using namespace Common;

namespace Brigades {

namespace AI {

SoldierController::SoldierController(SoldierPtr p)
	: Brigades::SoldierController(p),
	mCommandTimer(1.0f)
{
}

void SoldierController::act(float time)
{
	if(handleEvents() || mSoldier->getSensorySystem()->update(time)) {
		updateTargetSoldier();
	}

	if(!mSoldier->getCommandees().empty() && mCommandTimer.check(time)) {
		updateCommandeeOrders();
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
			vel = mSteering->wander(2.0f, 10.0f, 3.0f);
		}
	}
	if(mSoldier->getCommandees().empty())
		vel.truncate(10.0f);
	else
		vel.truncate(0.02f);

	mSteering->accumulate(tot, vel);

	if(mWorld->teamWon() < 0) {
		std::vector<Entity*> neighbours;
		auto leader = mSoldier->getLeader();
		if(leader && leader->isDead()) {
			leader = SoldierPtr();
			mSoldier->setLeader(leader);
		}
		bool leaderVisible = false;
		bool beingLead = false;

		for(auto n : mSoldier->getSensorySystem()->getSoldiers()) {
			if(n->getSideNum() == mSoldier->getSideNum()) {
				neighbours.push_back(n.get());
				if(leader && !leaderVisible && n == leader)
					leaderVisible = true;
			}
		}

		beingLead = leader && !mSoldier->getFormationOffset().null() && leaderVisible;

		if(beingLead) {
			Vector3 sep = mSteering->separation(neighbours);
			sep.truncate(10.0f);
			if(mSteering->accumulate(tot, sep)) {
				Vector3 coh = mSteering->cohesion(neighbours);
				coh.truncate(1.5f);
				mSteering->accumulate(tot, coh);
			}

			Vector3 offset = mSteering->offsetPursuit(*leader, mSoldier->getFormationOffset());
			mSteering->accumulate(tot, offset);
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

void SoldierController::updateCommandeeOrders()
{
	mSoldier->pruneCommandees();
	mSoldier->setLineFormation(10.0f);
}

}

}

