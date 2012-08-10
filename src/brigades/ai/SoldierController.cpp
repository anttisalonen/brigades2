#include <cassert>
#include <cfloat>
#include <climits>
#include <stdexcept>

#include "brigades/SensorySystem.h"

#include "brigades/ai/SoldierController.h"

using namespace Common;

namespace Brigades {

namespace AI {

Goal::Goal(SoldierPtr s)
	: mSoldier(s),
	mWorld(s->getWorld())
{
}

AtomicGoal::AtomicGoal(SoldierPtr s)
	: Goal(s)
{
}

void AtomicGoal::addSubGoal(GoalPtr g)
{
	assert(0);
	throw std::runtime_error("AtomicGoal::addSubGoal: illegal");
}

CompositeGoal::CompositeGoal(SoldierPtr s)
	: Goal(s)
{
}

void CompositeGoal::addSubGoal(GoalPtr g)
{
	mSubGoals.push_front(g);
}

SeekAndDestroyGoal::SeekAndDestroyGoal(SoldierPtr s)
	: AtomicGoal(s),
	mTargetUpdateTimer(0.25f),
	mCommandTimer(1.0f),
	mRetreat(false)
{
}

void SeekAndDestroyGoal::activate()
{
}

bool SeekAndDestroyGoal::process(float time)
{
	if(mTargetUpdateTimer.check(time)) {
		updateTargetSoldier();
	}

	if(!mSoldier->getCommandees().empty() && mCommandTimer.check(time)) {
		updateCommandeeOrders();
	}

	updateShootTarget();

	move(time);
	tryToShoot();

	return true;
}

void SeekAndDestroyGoal::deactivate()
{
}

void SeekAndDestroyGoal::updateCommandeeOrders()
{
	mSoldier->pruneCommandees();
	mSoldier->setLineFormation(10.0f);
}

void SeekAndDestroyGoal::updateShootTarget()
{
	if(!mTargetSoldier || !mSoldier->getCurrentWeapon()) {
		mShootTargetPosition = Vector3();
		return;
	}

	Vector3 pos = mTargetSoldier->getPosition();
	Vector3 vel = mTargetSoldier->getVelocity();

	float time1, time2;

	Vector3 topos = pos - mSoldier->getPosition();

	if(!Math::tps(topos, vel - mSoldier->getVelocity(), mSoldier->getCurrentWeapon()->getVelocity(), time1, time2)) {
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

void SeekAndDestroyGoal::move(float time)
{
	Vector3 vel;

	auto controller = mSoldier->getController();
	auto steering = controller->getSteering();
	Vector3 tot = controller->defaultMovement(time);

	if(mTargetSoldier) {
		if(!mRetreat) {
			vel = steering->pursuit(*mTargetSoldier);
		} else {
			vel = steering->evade(*mTargetSoldier);
		}
	} else {
		if(mWorld->teamWon() < 0) {
			vel = steering->wander(2.0f, 10.0f, 3.0f);
		}
	}
	if(mSoldier->getCommandees().empty())
		vel.truncate(10.0f);
	else
		vel.truncate(0.02f);

	steering->accumulate(tot, vel);

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
			Vector3 sep = steering->separation(neighbours);
			sep.truncate(10.0f);
			if(steering->accumulate(tot, sep)) {
				Vector3 coh = steering->cohesion(neighbours);
				coh.truncate(1.5f);
				steering->accumulate(tot, coh);
			}

			Vector3 offset = steering->offsetPursuit(*leader, mSoldier->getFormationOffset());
			steering->accumulate(tot, offset);
		}
	}

	controller->moveTo(tot, time, mShootTargetPosition.null());
	if(!mShootTargetPosition.null()) {
		controller->turnTo(mShootTargetPosition);
	}
}

void SeekAndDestroyGoal::updateTargetSoldier()
{
	auto soldiers = mSoldier->getSensorySystem()->getSoldiers();
	float distToNearest = FLT_MAX;
	SoldierPtr nearest;
	unsigned int weapontouse = INT_MAX;

	for(auto s : soldiers) {
		if(!s->isDead() && s->getSideNum() != mSoldier->getSideNum()) {
			float thisdist = Entity::distanceBetween(*mSoldier, *s);
			if(thisdist < distToNearest) {
				unsigned int i = 0;
				unsigned int bestweapon = INT_MAX;
				float bestscore = 0.0f;
				float rangeToTgt = Entity::distanceBetween(*mSoldier, *s);
				for(auto w : mSoldier->getWeapons()) {
					if(w->getRange() < rangeToTgt && bestscore > 0.0f)
						continue;

					float thisscore = s->damageFactorFromWeapon(w) * w->getVelocity() / std::max(0.01f, w->getLoadTime());
					if(thisscore > bestscore) {
						bestweapon = i;
						bestscore = thisscore;
					}
					i++;
				}
				if(distToNearest == FLT_MAX || bestscore > 0.0f) {
					distToNearest = thisdist;
					nearest = s;
					weapontouse = bestweapon;
				}
			}
		}
	}

	if(nearest) {
		if(weapontouse == INT_MAX) {
			mRetreat = true;
		}
		else {
			mRetreat = false;
			mSoldier->switchWeapon(weapontouse);
		}
		mTargetSoldier = nearest;
	}
	else {
		mTargetSoldier = SoldierPtr();
	}
}

void SeekAndDestroyGoal::tryToShoot()
{
	if(!mTargetSoldier || mRetreat || !mSoldier->getCurrentWeapon()) {
		return;
	}

	float dist = mShootTargetPosition.length();
	if(dist < mSoldier->getCurrentWeapon()->getRange()) {
		if(mSoldier->getCurrentWeapon()->canShoot()) {
			mSoldier->getCurrentWeapon()->shoot(mWorld, mSoldier, mShootTargetPosition);
		}
	}
}

SoldierController::SoldierController(SoldierPtr p)
	: Brigades::SoldierController(p),
	mCurrentGoal(GoalPtr(new SeekAndDestroyGoal(p)))
{
}

void SoldierController::act(float time)
{
	mCurrentGoal->process(time);
}

}

}

