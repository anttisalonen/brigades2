#include <cassert>
#include <cfloat>
#include <climits>
#include <stdexcept>
#include <queue>
#include <set>

#include "common/Random.h"

#include "brigades/SensorySystem.h"
#include "brigades/DebugOutput.h"
#include "brigades/InfoChannel.h"

#include "brigades/ai/SoldierController.h"

using namespace Common;

namespace Brigades {

namespace AI {

static Common::Vector3 sectorMiddlepoint(const Common::Rectangle& r)
{
	return Vector3(r.x + r.w * 0.5f, r.y + r.h * 0.5f, 0.0);
}

SectorMap::SectorMap(float width, float height, int dimx, int dimy)
{
	mWidth = width;
	mHeight = height;

	mDimX = dimx;
	mDimY = dimy;

	mNumXSectors = width / dimx;
	mNumYSectors = height / dimy;

	int size = mNumXSectors * mNumYSectors;
	mSectorMap.resize(size, 0);
}

signed char SectorMap::getValue(const Common::Vector3& v) const
{
	return mSectorMap.at(coordinateToIndex(v));
}

signed char SectorMap::getValue(const std::pair<unsigned int, unsigned int>& s) const
{
	return mSectorMap.at(sectorToIndex(s.first, s.second));
}

void SectorMap::setValue(const Vector3& v, signed char x)
{
	mSectorMap.at(coordinateToIndex(v)) = x;
}

void SectorMap::setAll(signed char c)
{
	for(auto& s : mSectorMap) {
		s = c;
	}
}

std::pair<unsigned int, unsigned int> SectorMap::coordinateToSector(const Vector3& v) const
{
	return std::make_pair((v.x + mWidth * 0.5f) / mDimX, (v.y + mHeight * 0.5f) / mDimY);
}

unsigned int SectorMap::sectorToIndex(unsigned int i, unsigned int j) const
{
	return j * mNumYSectors + i;
}

unsigned int SectorMap::sectorToIndex(const std::pair<unsigned int, unsigned int>& s) const
{
	return sectorToIndex(s.first, s.second);
}

unsigned int SectorMap::coordinateToIndex(const Vector3& v) const
{
	return sectorToIndex(coordinateToSector(v));
}

Vector3 SectorMap::sectorToCoordinate(const std::pair<unsigned int, unsigned int>& s) const
{
	Vector3 ret;
	ret.x = s.first * mDimX - mWidth * 0.5f + mDimY / 2;
	ret.y = s.second * mDimX - mHeight * 0.5f + mDimY / 2;
	assert(ret.x > -mWidth * 0.5f);
	assert(ret.x < mWidth * 0.5f);
	assert(ret.y > -mHeight * 0.5f);
	assert(ret.y < mHeight * 0.5f);
	return ret;
}

unsigned int SectorMap::getNumXSectors() const
{
	return mNumXSectors;
}

unsigned int SectorMap::getNumYSectors() const
{
	return mNumYSectors;
}

Common::Rectangle SectorMap::getSector(const std::pair<unsigned int, unsigned int>& s) const
{
	Vector3 lf = sectorToCoordinate(s);
	lf.x -= mDimX * 0.5f;
	lf.y -= mDimY * 0.5f;
	return Rectangle(lf.x, lf.y, mDimX, mDimY);
}

Common::Rectangle SectorMap::getSector(const Common::Vector3& v) const
{
	return getSector(coordinateToSector(v));
}


SubUnitHandler::SubUnitHandler(SoldierPtr s)
	: mSoldier(s)
{
}

bool SubUnitHandler::subUnitsReady() const
{
	for(auto c : mSubUnits) {
		if(c.second == SubUnitStatus::Attacking)
			return false;
	}
	return true;
}

void SubUnitHandler::updateSubUnitStatus()
{
	mSubUnits.clear();

	for(auto c : mSoldier->getCommandees()) {
		if(!mSoldier->canCommunicateWith(c))
			continue;

		if(c->defending())
			mSubUnits.insert({c, SubUnitStatus::Defending});
		else
			mSubUnits.insert({c, SubUnitStatus::Attacking});
	}
}

std::map<SoldierPtr, SubUnitStatus>& SubUnitHandler::getStatus()
{
	return mSubUnits;
}

const std::map<SoldierPtr, SubUnitStatus>& SubUnitHandler::getStatus() const
{
	return mSubUnits;
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

bool Goal::handleAttackSuccess(SoldierPtr s, const Common::Rectangle& r)
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

void SquadLeaderGoal::activate()
{
	if(!mSoldier->getLeader() || !mSoldier->canCommunicateWith(mSoldier->getLeader())) {
		mSoldier->giveAttackOrder(mWorld->getArea());
	}
}

bool SquadLeaderGoal::process(float time)
{
	static const float minDistToTgt = 10.0f;
	activateIfInactive();
	if(mSubGoals.empty()) {
		if(mSoldier->defending()) {
			mArea.x = mSoldier->getPosition().x - 16;
			mArea.y = mSoldier->getPosition().y - 16;
			mArea.w = mArea.h = 32;
			mSubGoals.push_front(GoalPtr(new SeekAndDestroyGoal(mSoldier, mArea)));
		} else {
			mArea = mSoldier->getAttackArea();
			mArea.x += mArea.w * 0.5f - minDistToTgt * 0.5f;
			mArea.y += mArea.h * 0.5f - minDistToTgt * 0.5f;
			mArea.w = minDistToTgt * 0.5f;
			mArea.h = minDistToTgt * 0.5f;
			mSubGoals.push_front(GoalPtr(new SeekAndDestroyGoal(mSoldier, mArea)));
		}
	} else if(!mSoldier->defending() && !mSoldier->hasEnemyContact()) {
		Vector3 tgt = sectorMiddlepoint(mArea);
		float distToTgt = mSoldier->getPosition().distance(tgt);
		if(distToTgt < minDistToTgt && mSoldier->getLeader()) {
			if(mSoldier->canCommunicateWith(mSoldier->getLeader())) {
				commandDefendPositions();
				mSoldier->setDefending();
				InfoChannel::getInstance()->say(mSoldier, "Reporting successful attack");
				if(!mSoldier->reportSuccessfulAttack()) {
					/* TODO */
				}
			} else {
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

void SquadLeaderGoal::commandDefendPositions()
{
	if(mSoldier->getCommandees().empty()) {
		return;
	}

	auto& defarea = mSoldier->getAttackArea();
	Vector3 midp(mSoldier->getPosition());
	float rad = std::max(defarea.w, defarea.h) * 0.5f;
	Vector3 towardsopp = mWorld->getHomeBasePosition(!mSoldier->getSide()->isFirst()) - midp;
	towardsopp.normalize();
	towardsopp *= rad;

	int numCommandees = mSoldier->getCommandees().size();
	if(numCommandees == 1) {
		(*(mSoldier->getCommandees().begin()))->setDefendPosition(midp + towardsopp);
	} else if(numCommandees == 2) {
		towardsopp = Math::rotate2D(towardsopp, QUARTER_PI * 0.5f);
		float ang = 0.0f;
		for(auto c : mSoldier->getCommandees()) {
			c->setDefendPosition(midp + Math::rotate2D(towardsopp, ang));
			ang -= QUARTER_PI;
		}
	} else {
		towardsopp = Math::rotate2D(towardsopp, HALF_PI);
		float ang = 0.0f;

		for(auto c : mSoldier->getCommandees()) {
			c->setDefendPosition(midp + Math::rotate2D(towardsopp, ang));
			ang -= PI / (numCommandees - 1);
		}
	}
}

PlatoonLeaderGoal::PlatoonLeaderGoal(SoldierPtr s)
	: CompositeGoal(s),
	mSubUnitHandler(s),
	mSubTimer(5.0f)
{
}

void PlatoonLeaderGoal::activate()
{
}

bool PlatoonLeaderGoal::process(float time)
{
	activateIfInactive();
	if(mSubTimer.check(time)) {
		mSubUnitHandler.updateSubUnitStatus();
		if(mSubUnitHandler.subUnitsReady()) {
			handleAttackFinish();
		}
	}
	if(mSubGoals.empty()) {
		mSubGoals.push_front(GoalPtr(new SeekAndDestroyGoal(mSoldier,
						Rectangle(mSoldier->getPosition().x - 16, mSoldier->getPosition().y - 16, 32, 32))));
	}
	return processSubGoals(time);
}

bool PlatoonLeaderGoal::handleAttackSuccess(SoldierPtr s, const Common::Rectangle& r)
{
	if(!mSoldier->getLeader() || !mSoldier->canCommunicateWith(mSoldier->getLeader()))
		return false;

	mSubUnitHandler.updateSubUnitStatus();
	if(mSubUnitHandler.subUnitsReady()) {
		handleAttackFinish();
	}

	return true;
}

bool PlatoonLeaderGoal::handleAttackOrder(const Common::Rectangle& r)
{
	mTargetRectangle = r;

	mSubUnitHandler.updateSubUnitStatus();
	std::vector<Rectangle> rects = splitTargetRectangle();
	assert(rects.size() == mSubUnitHandler.getStatus().size());

	int i = 0;
	for(auto p : mSubUnitHandler.getStatus()) {
		p.first->giveAttackOrder(rects[i++]);
	}

	return true;
}

std::vector<Common::Rectangle> PlatoonLeaderGoal::splitTargetRectangle() const
{
	std::vector<Common::Rectangle> ret;
	int numgroups = mSubUnitHandler.getStatus().size();
	if(numgroups == 1) {
		ret.push_back(mTargetRectangle);
	} else if(numgroups == 4 && mTargetRectangle.w * 2.0f > mTargetRectangle.h &&
			mTargetRectangle.h * 2.0f > mTargetRectangle.w) {
		Rectangle r1(mTargetRectangle);
		r1.w *= 0.5f;
		r1.h *= 0.5f;
		Rectangle r2(r1);
		r2.x += r1.w;
		Rectangle r3(r1);
		r3.y += r1.h;
		Rectangle r4(r2);
		r4.y += r1.h;
		ret.push_back(r1);
		ret.push_back(r2);
		ret.push_back(r3);
		ret.push_back(r4);
	} else {
		for(int i = 0; i < numgroups; i++) {
			Rectangle r1(mTargetRectangle);
			if(mTargetRectangle.w >= mTargetRectangle.h) {
				r1.w *= 1.0f / (float)numgroups;
				r1.x += i * r1.w;
				ret.push_back(r1);
			} else {
				r1.h *= 1.0f / (float)numgroups;
				r1.y += i * r1.h;
				ret.push_back(r1);
			}
		}
	}

	return ret;
}

void PlatoonLeaderGoal::handleAttackFinish()
{
	if(!mSoldier->defending()) {
		mSoldier->setDefending();
		if(mSoldier->getLeader() && !mSoldier->reportSuccessfulAttack()) {
			assert(0);
		}
	}
}


CompanyLeaderGoal::CompanyLeaderGoal(SoldierPtr s)
	: CompositeGoal(s),
	mSectorMap(SectorMap(mWorld->getWidth(), mWorld->getHeight(), mWorld->getWidth() / 4,
				mWorld->getHeight() / 4)),
	mSubUnitHandler(s),
	mSubTimer(5.0f)
{
}

void CompanyLeaderGoal::activate()
{
	updateSectorMap();
	mSubUnitHandler.updateSubUnitStatus();
	issueAttackOrders();
}

bool CompanyLeaderGoal::process(float time)
{
	activateIfInactive();
	if(mSubTimer.check(time)) {
		mSubUnitHandler.updateSubUnitStatus();
		if(mSubUnitHandler.subUnitsReady()) {
			handleAttackFinish();
		}
	}
	if(mSubGoals.empty()) {
		mSubGoals.push_front(GoalPtr(new SeekAndDestroyGoal(mSoldier,
						Rectangle(mSoldier->getPosition().x - 16, mSoldier->getPosition().y - 16, 32, 32))));
	}
	for(auto c : mSubUnitHandler.getStatus()) {
		DebugOutput::getInstance()->markArea(mSoldier->getSideNum() == 0 ? Common::Color::Red : Common::Color::Blue,
				c.first->getAttackArea(), false);
	}
	return processSubGoals(time);
}

bool CompanyLeaderGoal::handleAttackSuccess(SoldierPtr s, const Common::Rectangle& r)
{
	mSubUnitHandler.updateSubUnitStatus();
	if(mSubUnitHandler.subUnitsReady()) {
		handleAttackFinish();
	}

	return true;
}

std::set<Common::Rectangle> CompanyLeaderGoal::nextAttackSectors() const
{
	std::set<Common::Rectangle> ret;
	for(auto c : mSubUnitHandler.getStatus()) {
		if(!c.first->canCommunicateWith(c.first))
			continue;

		if(!c.first->defending())
			continue;

		Vector3 exppoint;
		if(c.first->getAttackArea().w == 0) {
			// not initialised
			exppoint = c.first->getPosition();
		} else {
			exppoint = sectorMiddlepoint(c.first->getAttackArea());
		}

		for(int j = -1; j <= 1; j++) {
			for(int i = -1; i <= 1; i++) {
				if(abs(i) == abs(j) && (i || j))
					continue;

				auto s = mSectorMap.coordinateToSector(exppoint);
				if((s.first == 0 && i < 0) || (s.second == 0 && j < 0))
					continue;

				s.first += i;
				s.second += j;

				if(s.first >= mSectorMap.getNumXSectors() ||
						s.second >= mSectorMap.getNumYSectors())
					continue;

				if(mSectorMap.getValue(s))
					continue;

				ret.insert(mSectorMap.getSector(s));
			}
		}
	}

	if(ret.empty()) {
		for(int j = 0; j < mSectorMap.getNumYSectors(); j++) {
			for(int i = 0; i < mSectorMap.getNumYSectors(); i++) {
				if(!mSectorMap.getValue({i, j})) {
					ret.insert(mSectorMap.getSector({i, j}));
				}
			}
		}
	}

	return ret;
}

void CompanyLeaderGoal::updateSectorMap()
{
	for(auto c : mSubUnitHandler.getStatus()) {
		if(!mSoldier->canCommunicateWith(c.first))
			continue;

		if(c.first->defending() && c.first->getAttackArea().w) {
			mSectorMap.setValue(sectorMiddlepoint(c.first->getAttackArea()), 1);
		}
	}
}

void CompanyLeaderGoal::issueAttackOrders()
{
	auto rs = nextAttackSectors();
	if(rs.empty())
		return;

	InfoChannel::getInstance()->say(mSoldier, "Issuing company attack orders");

	auto rsit = rs.begin();

	/* TODO: set up some smarter way for distributing sectors to platoons. */
	for(auto c : mSubUnitHandler.getStatus()) {
		if(c.first->giveAttackOrder(*rsit)) {
			rsit++;
			if(rsit == rs.end()) {
				break;
			}
		} else {
			std::cout << "AI: platoon unable to comply to attack order.\n";
		}
	}
}

void CompanyLeaderGoal::handleAttackFinish()
{
	updateSectorMap();
	issueAttackOrders();
}


SeekAndDestroyGoal::SeekAndDestroyGoal(SoldierPtr s, const Common::Rectangle& r)
	: AtomicGoal(s),
	mTargetUpdateTimer(0.25f),
	mCommandTimer(1.0f),
	mRetreat(false),
	mArea(r),
	mBoredTimer(Random::uniform() * 30.0f + 10.0f),
	mEnoughWanderTimer(Random::uniform() * 10.0f + 5.0f),
	mAssaultTimer(Random::uniform() * 5.0f + 5.0f)
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
	if(!mSoldier->defending())
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
	bool digging = false;

	auto controller = mSoldier->getController();
	auto steering = controller->getSteering();
	Vector3 tot = controller->defaultMovement(time);

	if(mTargetSoldier) {
		if(!mRetreat) {
			// decide whether to shoot or advance here
			auto foxhole = mWorld->getFoxholeAt(mSoldier->getPosition());
			bool dugIn = mSoldier->getWarriorType() == WarriorType::Soldier &&
				foxhole && foxhole->getDepth() > 0.4f;
			float dist = mShootTargetPosition.length();
			float distCoeff = dugIn ? 0.8f : 0.5f;
			if((!dugIn && !mAssaultTimer.running()) || !mSoldier->getCurrentWeapon()->speedVariates() ||
					dist * distCoeff > mSoldier->getCurrentWeapon()->getRange()) {
				vel = steering->pursuit(*mTargetSoldier);
				InfoChannel::getInstance()->say(mSoldier, "Assaulting");
			} else {
				mAssaultTimer.doCountdown(time);
				mAssaultTimer.check();
			}
		} else {
			vel = steering->evade(*mTargetSoldier);
		}
	} else {
		if(mWorld->teamWon() < 0) {
			bool alone = (!mSoldier->getLeader() || !mSoldier->canCommunicateWith(mSoldier->getLeader())) &&
				mSoldier->getRank() <= SoldierRank::Sergeant;
			if(!mArea.w || !mArea.h || mArea.pointWithin(mSoldier->getPosition().x, mSoldier->getPosition().y) ||
					alone) {
				if(alone || mEnoughWanderTimer.running()) {
					vel = steering->wander(2.0f, 10.0f, 3.0f);
					if(!alone) {
						mEnoughWanderTimer.doCountdown(time);
						if(mEnoughWanderTimer.check()) {
							mBoredTimer.rewind();
						}
					}
				} else {
					mBoredTimer.doCountdown(time);
					if(mBoredTimer.check()) {
						mEnoughWanderTimer.rewind();
					}
				}
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
		bool leaderVisible = leader && mSoldier->canCommunicateWith(leader);
		bool beingLead = false;

		for(auto n : mSoldier->getSensorySystem()->getSoldiers()) {
			if(n->getSideNum() == mSoldier->getSideNum()) {
				neighbours.push_back(n.get());
			}
		}

		beingLead = mSoldier->getRank() < SoldierRank::Sergeant &&
			leaderVisible && (!mSoldier->getFormationOffset().null() ||
					!mSoldier->getDefendPosition().null());

		if(beingLead) {
			Vector3 sep = steering->separation(neighbours);
			sep.truncate(10.0f);
			if(sep.length2() > 1.0f && steering->accumulate(tot, sep)) {
				Vector3 coh = steering->cohesion(neighbours);
				coh.truncate(1.5f);
				if(coh.length2() > 1.0f)
					steering->accumulate(tot, coh);
			}

			if(!mSoldier->getFormationOffset().null()) {
				Vector3 offset = steering->offsetPursuit(*leader, mSoldier->getFormationOffset());
				steering->accumulate(tot, offset);
			} else {
				float dist2 = mSoldier->getPosition().distance2(mSoldier->getDefendPosition());
				if(dist2 > 2.0f) {
					Vector3 arr = steering->arrive(mSoldier->getDefendPosition());
					steering->accumulate(tot, arr);
				} else if(mShootTargetPosition.null()) {
					mSoldier->dig(time);
					digging = true;
				}
			}
		}
	}

	if(!digging) {
		controller->moveTo(tot, time, mShootTargetPosition.null());
		DebugOutput::getInstance()->addArrow(mSoldier->getSideNum() == 0 ? Common::Color::Red : Common::Color::Blue,
				mSoldier->getPosition(), tot + mSoldier->getPosition());

		if(!mShootTargetPosition.null()) {
			controller->turnTo(mShootTargetPosition);
		}
	}
}

void SeekAndDestroyGoal::updateTargetSoldier()
{
	auto soldiervec = mSoldier->getSensorySystem()->getSoldiers();
	std::set<SoldierPtr> soldiers;
	soldiers.insert(soldiervec.begin(), soldiervec.end());
	if(mSoldier->getLeader()) {
		auto ss = mSoldier->getLeader()->getSensorySystem()->getSoldiers();
		soldiers.insert(ss.begin(), ss.end());
		for(auto s : mSoldier->getLeader()->getCommandees()) {
			auto ss = s->getSensorySystem()->getSoldiers();
			soldiers.insert(ss.begin(), ss.end());
		}
	}
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
		if(mTargetSoldier != nearest) {
			mAssaultTimer.rewind();
			mTargetSoldier = nearest;
		}
	}
	else {
		mTargetSoldier = SoldierPtr();
		mAssaultTimer.rewind();
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
	resetRootGoal();
}

void SoldierController::resetRootGoal()
{
	switch(mSoldier->getRank()) {
		case SoldierRank::Private:
			mCurrentGoal = GoalPtr(new PrivateGoal(mSoldier));
			break;

		case SoldierRank::Sergeant:
			mCurrentGoal = GoalPtr(new SquadLeaderGoal(mSoldier));
			break;

		case SoldierRank::Lieutenant:
			mCurrentGoal = GoalPtr(new PlatoonLeaderGoal(mSoldier));
			break;

		case SoldierRank::Captain:
			mCurrentGoal = GoalPtr(new CompanyLeaderGoal(mSoldier));
			break;
	}
}

void SoldierController::act(float time)
{
	if(handleLeaderCheck(time)) {
		resetRootGoal();
	}
	mCurrentGoal->process(time);
}

bool SoldierController::handleAttackOrder(const Rectangle& r)
{
	return mCurrentGoal->handleAttackOrder(r);
}

bool SoldierController::handleAttackSuccess(SoldierPtr s, const Common::Rectangle& r)
{
	return mCurrentGoal->handleAttackSuccess(s, r);
}

}

}

