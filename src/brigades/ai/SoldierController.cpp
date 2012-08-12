#include <cassert>
#include <cfloat>
#include <climits>
#include <stdexcept>
#include <queue>
#include <set>

#include "brigades/SensorySystem.h"
#include "brigades/DebugOutput.h"

#include "brigades/ai/SoldierController.h"

using namespace Common;

namespace Brigades {

namespace AI {

static Common::Vector3 sectorMiddlepoint(const Common::Rectangle& r)
{
	return Vector3(r.x + r.w * 0.5f, r.y + r.h * 0.5f, 0.0);
}

Goal::Goal(SoldierPtr s)
	: mSoldier(s),
	mWorld(s->getWorld())
{
}

bool Goal::handleAttackOrder(const Common::Rectangle& r)
{
	return false;
}

bool Goal::handleAttackSuccess(const Common::Rectangle& r)
{
	return false;
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
	: Goal(s),
	mActive(false)
{
}

void CompositeGoal::addSubGoal(GoalPtr g)
{
	mSubGoals.push_front(g);
}

bool CompositeGoal::processSubGoals(float time)
{
	if(!mSubGoals.empty()) {
		bool running = mSubGoals.front()->process(time);
		if(running) {
			return true;
		} else {
			mSubGoals.front()->deactivate();
			mSubGoals.pop_front();
			if(!mSubGoals.empty()) {
				/* TODO: activate() won't be called for the first subgoal.
				 * Current remedy is to call activateIfInactive() at the
				 * start of each process(). */
				mSubGoals.front()->activate();
				return true;
			}
		}
	}
	return false;
}

void CompositeGoal::emptySubGoals()
{
	if(!mSubGoals.empty()) {
		mSubGoals.front()->deactivate();
		mSubGoals.clear();
	}
}

void CompositeGoal::activateIfInactive()
{
	if(!mActive) {
		activate();
		mActive = true;
	}
}

void CompositeGoal::deactivate()
{
	mActive = false;
}

PrivateGoal::PrivateGoal(SoldierPtr s)
	: CompositeGoal(s)
{
}

bool PrivateGoal::process(float time)
{
	activateIfInactive();
	if(mSubGoals.empty()) {
		mSubGoals.push_front(GoalPtr(new SeekAndDestroyGoal(mSoldier, Rectangle())));
	}
	return processSubGoals(time);
}

bool PrivateGoal::handleAttackOrder(const Rectangle& r)
{
	emptySubGoals();
	return true;
}

SquadLeaderGoal::SquadLeaderGoal(SoldierPtr s)
	: CompositeGoal(s)
{
}

bool SquadLeaderGoal::process(float time)
{
	activateIfInactive();
	if(mSubGoals.empty()) {
		if(mSoldier->defending()) {
			mArea.x = mSoldier->getPosition().x - 16;
			mArea.y = mSoldier->getPosition().y - 16;
			mArea.w = mArea.h = 32;
			mSubGoals.push_front(GoalPtr(new SeekAndDestroyGoal(mSoldier, mArea)));
		} else {
			mArea = mSoldier->getAttackArea();
			mSubGoals.push_front(GoalPtr(new SeekAndDestroyGoal(mSoldier, mArea)));
		}
	} else if(!mSoldier->defending() && !mSoldier->hasEnemyContact()) {
		Vector3 tgt = sectorMiddlepoint(mArea);
		float distToTgt = mSoldier->getPosition().distance(tgt);
		if(distToTgt < 10.0f) {
			if(mSoldier->canCommunicateWith(mSoldier->getLeader())) {
				mSoldier->setDefending();
				mSoldier->getLeader()->reportSuccessfulAttack(mArea);
			} else {
				/* TODO */
			}
		}
	}
	return processSubGoals(time);
}

bool SquadLeaderGoal::handleAttackOrder(const Rectangle& r)
{
	emptySubGoals();
	return true;
}

PlatoonLeaderGoal::PlatoonLeaderGoal(SoldierPtr s)
	: CompositeGoal(s)
{
	mNumXSectors = mWorld->getHeight() / 64;
	mNumYSectors = mWorld->getWidth() / 64;

	int size = mNumXSectors * mNumYSectors;
	mSectorMap.resize(size, -1);
	bool first = s->getSide()->isFirst();
	const Vector3& awaypos = mWorld->getHomeBasePosition(!first);
	markEnemy(awaypos);
}

void PlatoonLeaderGoal::activate()
{
	for(auto s : mSoldier->getCommandees()) {
		if(mSoldier->canCommunicateWith(s)) {
			const Vector3& pos = s->getPosition();
			if(s->hasEnemyContact()) {
				markCombat(pos);
			} else {
				markHome(pos);
			}
		}
	}

	giveOrders();
}

bool PlatoonLeaderGoal::process(float time)
{
	activateIfInactive();
	if(mSubGoals.empty()) {
		mSubGoals.push_front(GoalPtr(new SeekAndDestroyGoal(mSoldier, getSector(mSoldier->getPosition()))));
	}
	return processSubGoals(time);
}

bool PlatoonLeaderGoal::handleAttackSuccess(const Common::Rectangle& r)
{
	Vector3 p = sectorMiddlepoint(r);
	markHome(p);
	{
		auto s = coordinateToSector(p);
		std::cout << mSoldier->getSideNum() << ": Sector successfully claimed: (" << s.first << ", " << s.second << ")\n";
	}
	giveOrders();
	return true;
}

void PlatoonLeaderGoal::markHome(const Vector3& v)
{
	unsigned int index = coordinateToIndex(v);
	signed char& i = mSectorMap.at(index);
	if(i > 0 && i < 100)
		i++;
	else
		i = 1;
}

void PlatoonLeaderGoal::reduceHome(const Vector3& v)
{
	signed char& i = mSectorMap.at(coordinateToIndex(v));
	assert(i > 0);
	i--;
}

void PlatoonLeaderGoal::markEnemy(const Vector3& v)
{
	signed char& i = mSectorMap.at(coordinateToIndex(v));
	if(i < 0 && i > -100)
		i--;
	else
		i = -1;
}

void PlatoonLeaderGoal::markCombat(const Vector3& v)
{
	mSectorMap.at(coordinateToIndex(v)) = 0;
}

void PlatoonLeaderGoal::giveOrders()
{
	for(auto s : mSoldier->getCommandees()) {
		if(mSoldier->canCommunicateWith(s)) {
			if(s->defending()) {
				Vector3 attackpos;
				if(calculateAttackPosition(s, attackpos)) {
					DebugOutput::getInstance()->markArea(mSoldier->getSideNum() == 0 ? Common::Color::Red : Common::Color::Blue,
							getSector(attackpos), false);
					s->giveAttackOrder(getSector(attackpos));
					reduceHome(s->getPosition());
					markCombat(attackpos);
				}
			}
		}
	}
}

bool PlatoonLeaderGoal::calculateAttackPosition(const SoldierPtr s, Vector3& pos)
{
	if(criticalSquad(s)) {
		return false;
	}

	int shortestdist = INT_MAX;

	std::pair<unsigned int, unsigned int> startpos = coordinateToSector(s->getPosition());
	std::pair<unsigned int, unsigned int> bestpos = startpos;

	std::queue<std::pair<unsigned int, unsigned int>> freespots;

	std::set<std::pair<unsigned int, unsigned int>> visited;

	freespots.push(startpos);

	while(!freespots.empty()) {
		auto thispos = freespots.front();
		visited.insert(thispos);
		if(mSectorMap.at(sectorToIndex(thispos)) < 1) {
			// neighbouring cell occupied by enemy
			int thisdist = (sectorToCoordinate(thispos) - mWorld->getHomeBasePosition(!mSoldier->getSide()->isFirst())).length();
			if(thisdist < shortestdist) {
				shortestdist = thisdist;
				bestpos = thispos;
			}
			freespots.pop();
			continue;
		}

		for(int j = -1; j <= 1; j++) {
			for(int i = -1; i <= 1; i++) {
				auto p = std::make_pair(thispos.first + i, thispos.second + j);
				if(visited.find(p) != visited.end())
					continue;

				if((abs(i) == 0) == (abs(j) == 0))
					continue;

				if(thispos.first == 0 && i == -1)
					continue;

				if(thispos.first == mNumXSectors - 1 && i ==  1)
					continue;

				if(thispos.second == 0 && j == -1)
					continue;

				if(thispos.second == mNumYSectors - 1 && j ==  1)
					continue;

				freespots.push(p);
			}
		}
		freespots.pop();
	}

	if(shortestdist < INT_MAX) {
		pos = sectorToCoordinate(bestpos);
		std::cout << mSoldier->getSideNum() << ": Order to attack (" << bestpos.first << ", " << bestpos.second << ")\n";
		return true;
	} else {
		return false;
	}
}

std::pair<unsigned int, unsigned int> PlatoonLeaderGoal::coordinateToSector(const Vector3& v) const
{
	return std::make_pair((v.x + mWorld->getWidth() * 0.5f) / 64, (v.y + mWorld->getHeight() * 0.5f) / 64);
}

unsigned int PlatoonLeaderGoal::sectorToIndex(unsigned int i, unsigned int j) const
{
	return j * mNumYSectors + i;
}

unsigned int PlatoonLeaderGoal::sectorToIndex(const std::pair<unsigned int, unsigned int>& s) const
{
	return sectorToIndex(s.first, s.second);
}

unsigned int PlatoonLeaderGoal::coordinateToIndex(const Vector3& v) const
{
	return sectorToIndex(coordinateToSector(v));
}

Vector3 PlatoonLeaderGoal::sectorToCoordinate(const std::pair<unsigned int, unsigned int>& s) const
{
	Vector3 ret;
	ret.x = s.first * 64 - mWorld->getWidth() * 0.5f + 64 / 2;
	ret.y = s.second * 64 - mWorld->getHeight() * 0.5f + 64 / 2;
	assert(ret.x > -mWorld->getWidth() * 0.5f);
	assert(ret.x < mWorld->getWidth() * 0.5f);
	assert(ret.y > -mWorld->getHeight() * 0.5f);
	assert(ret.y < mWorld->getHeight() * 0.5f);
	return ret;
}

bool PlatoonLeaderGoal::criticalSquad(const SoldierPtr p) const
{
	/* TODO: rewrite the function so that it reports whether a hole in the line
	 * would be created or the dictator left unprotected. */
	bool first = mSoldier->getSide()->isFirst();
	const Vector3& homepos = mWorld->getHomeBasePosition(first);
	return coordinateToIndex(p->getPosition()) == coordinateToIndex(homepos) &&
		mSectorMap.at(coordinateToIndex(p->getPosition())) == 1;
}

Common::Rectangle PlatoonLeaderGoal::getSector(const std::pair<unsigned int, unsigned int>& s) const
{
	Vector3 lf = sectorToCoordinate(s);
	lf.x -= 32.0f;
	lf.y -= 32.0f;
	return Rectangle(lf.x, lf.y, 64, 64);
}

Common::Rectangle PlatoonLeaderGoal::getSector(const Common::Vector3& v) const
{
	return getSector(coordinateToSector(v));
}

SeekAndDestroyGoal::SeekAndDestroyGoal(SoldierPtr s, const Common::Rectangle& r)
	: AtomicGoal(s),
	mTargetUpdateTimer(0.25f),
	mCommandTimer(1.0f),
	mRetreat(false),
	mArea(r)
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
			if((!mArea.w && !mArea.h) ||
					(mArea.pointWithin(mSoldier->getPosition().x, mSoldier->getPosition().y))) {
				vel = steering->wander(2.0f, 10.0f, 3.0f);
			} else {
				Vector3 p = sectorMiddlepoint(mArea);
				vel = steering->arrive(p);
			}
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

		beingLead = mSoldier->getRank() < SoldierRank::Sergeant &&
			leader &&
			!mSoldier->getFormationOffset().null() && leaderVisible;

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
	DebugOutput::getInstance()->addArrow(mSoldier->getSideNum() == 0 ? Common::Color::Red : Common::Color::Blue,
			mSoldier->getPosition(), tot + mSoldier->getPosition());
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
	: Brigades::SoldierController(p)
{
	switch(p->getRank()) {
		case SoldierRank::Private:
		case SoldierRank::Corporal:
			mCurrentGoal = GoalPtr(new PrivateGoal(p));
			break;

		case SoldierRank::Sergeant:
			mCurrentGoal = GoalPtr(new SquadLeaderGoal(p));
			break;

		case SoldierRank::Lieutenant:
			mCurrentGoal = GoalPtr(new PlatoonLeaderGoal(p));
			break;
	}
}

void SoldierController::act(float time)
{
	mCurrentGoal->process(time);
}

bool SoldierController::handleAttackOrder(const Rectangle& r)
{
	return mCurrentGoal->handleAttackOrder(r);
}

bool SoldierController::handleAttackSuccess(const Common::Rectangle& r)
{
	return mCurrentGoal->handleAttackSuccess(r);
}

}

}

