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

#define SECTOR_OWN_PRESENCE	0x01
#define SECTOR_ENEMY_PRESENCE	0x02
#define SECTOR_PLANNED_ATTACK	0x04

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

signed char SectorMap::getValue(unsigned int i, unsigned int j) const
{
	return mSectorMap.at(sectorToIndex(i, j));
}

void SectorMap::setValue(const Vector3& v, signed char x)
{
	mSectorMap.at(coordinateToIndex(v)) = x;
}

void SectorMap::setBit(const Common::Vector3& v, signed char x)
{
	mSectorMap.at(coordinateToIndex(v)) |= x;
}

void SectorMap::clearBit(const Common::Vector3& v, signed char x)
{
	mSectorMap.at(coordinateToIndex(v)) &= ~x;
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

float SectorMap::getDistanceBetween(const std::pair<unsigned int, unsigned int>& s1,
		const std::pair<unsigned int, unsigned int>& s2) const
{
	int dx = int(s2.first) - int(s1.first);
	int dy = int(s2.second) - int(s1.second);
	return sqrt(dx * dx + dy * dy);
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

std::vector<std::pair<unsigned int, unsigned int>> SectorMap::getAdjacentSectors(const std::pair<unsigned int, unsigned int>& sec) const
{
	std::vector<std::pair<unsigned int, unsigned int>> ret;
	if(sec.first > 0) {
		auto s(sec);
		s.first--;
		ret.push_back(s);
	}
	if(sec.second > 0) {
		auto s(sec);
		s.second--;
		ret.push_back(s);
	}
	if(sec.first < mNumXSectors - 1) {
		auto s(sec);
		s.first++;
		ret.push_back(s);
	}
	if(sec.second < mNumYSectors - 1) {
		auto s(sec);
		s.second++;
		ret.push_back(s);
	}
	return ret;
}

std::vector<std::pair<unsigned int, unsigned int>> SectorMap::getAdjacentSectors(const Common::Vector3& v) const
{
	auto sec = coordinateToSector(v);
	return getAdjacentSectors(sec);
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
					std::cerr << "Unable to report successful attack!\n";
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
	if(mSubUnitHandler.subUnitsReady() && !mSoldier->getCommandees().empty()) {
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
	mSectorMap.setBit(mWorld->getHomeBasePosition(mSoldier->getSide()->isFirst()),
			SECTOR_OWN_PRESENCE);
}

void CompanyLeaderGoal::activate()
{
	updateSectorMap();
	mSubUnitHandler.updateSubUnitStatus();
}

bool CompanyLeaderGoal::process(float time)
{
	activateIfInactive();
	if(mSubTimer.check(time)) {
		updateSectorMap();
		mSubUnitHandler.updateSubUnitStatus();
		for(auto c : mSubUnitHandler.getStatus()) {
			if(c.second == SubUnitStatus::Defending) {
				tryUpdateDefensePosition(c.first);
			}
		}
	}
	if(mSubGoals.empty()) {
		mSubGoals.push_front(GoalPtr(new SeekAndDestroyGoal(mSoldier,
						Rectangle(mSoldier->getPosition().x - 16, mSoldier->getPosition().y - 16, 32, 32))));
	}

	if(mSoldier->getSideNum() == 0) {
		for(unsigned int j = 0; j < mSectorMap.getNumYSectors(); j++) {
			for(unsigned int i = 0; i < mSectorMap.getNumXSectors(); i++) {
				Common::Color c;
				auto v = mSectorMap.getValue({i, j});
				if(v == 0) {
					continue;
				}
				if(v & SECTOR_OWN_PRESENCE)
					c.g = 255;
				if(v & SECTOR_ENEMY_PRESENCE)
					c.r = 255;
				if(v & SECTOR_PLANNED_ATTACK)
					c.b = 255;
				DebugOutput::getInstance()->markArea(c, mSectorMap.getSector({i, j}), false);
			}
		}
	}

	return processSubGoals(time);
}

bool CompanyLeaderGoal::handleAttackSuccess(SoldierPtr s, const Common::Rectangle& r)
{
	// TODO: need notification when platoon is lost
	auto v = sectorMiddlepoint(r);

	mSectorMap.setBit(v, SECTOR_OWN_PRESENCE);
	mSectorMap.clearBit(v, SECTOR_ENEMY_PRESENCE);
	for(auto& s : mSectorMap.getAdjacentSectors(v)) {
		mSectorMap.clearBit(mSectorMap.sectorToCoordinate(s),
				SECTOR_ENEMY_PRESENCE);
	}
	mSectorMap.clearBit(v, SECTOR_PLANNED_ATTACK);

	return true;
}

void CompanyLeaderGoal::updateSectorMap()
{
	for(auto c : mSubUnitHandler.getStatus()) {
		if(!mSoldier->canCommunicateWith(c.first))
			continue;

		auto aa = c.first->getAttackArea();
		auto v = sectorMiddlepoint(aa);

		if(!aa.w)
			continue;

		if(c.first->hasEnemyContact()) {
			// TODO: should rather have an event from lieutenant
			// reporting enemy contact, and the position
			// (attack area is incorrect)
			mSectorMap.setBit(v, SECTOR_ENEMY_PRESENCE);
		} else if(c.first->defending() &&
				(mSectorMap.getValue(v) & SECTOR_OWN_PRESENCE) &&
				!c.first->getCommandees().empty()) {
			// repelled enemy offensive
			mSectorMap.clearBit(v, SECTOR_ENEMY_PRESENCE);
			for(auto& s : mSectorMap.getAdjacentSectors(v)) {
				mSectorMap.clearBit(mSectorMap.sectorToCoordinate(s),
						SECTOR_ENEMY_PRESENCE);
			}
		}
	}
}

void CompanyLeaderGoal::tryUpdateDefensePosition(SoldierPtr lieu)
{
	if(mSectorMap.getValue(sectorMiddlepoint(lieu->getAttackArea())) & SECTOR_ENEMY_PRESENCE)
		return;

	auto r2 = findNextDefensiveSector();
	auto vec = sectorMiddlepoint(r2);

	if(mSectorMap.coordinateToSector(sectorMiddlepoint(lieu->getAttackArea())) !=
		       mSectorMap.coordinateToSector(vec)) {
		if(!lieu->giveAttackOrder(r2)) {
			std::cerr << "AI: platoon unable to comply to attack order when updating defense position.\n";
		} else {
			// TODO: the lieutenant should report failure, in which case this bit should be cleared
			mSectorMap.setBit(vec, SECTOR_PLANNED_ATTACK);
		}
	}
}

bool CompanyLeaderGoal::isAdjacentToEnemy(const Common::Vector3& pos) const
{
	auto adj = mSectorMap.getAdjacentSectors(pos);
	for(auto& s : adj) {
		if(mSectorMap.getValue(s) & SECTOR_ENEMY_PRESENCE) {
			return true;
		}
	}

	return false;
}

Common::Rectangle CompanyLeaderGoal::findNextDefensiveSector() const
{
	auto homevec = mWorld->getHomeBasePosition(mSoldier->getSide()->isFirst());
	auto homesec = mSectorMap.coordinateToSector(homevec);
	float maxdist = FLT_MAX;
	std::pair<unsigned int, unsigned int> threatsec;

	for(unsigned int j = 0; j < mSectorMap.getNumYSectors(); j++) {
		for(unsigned int i = 0; i < mSectorMap.getNumXSectors(); i++) {
			// pick either a sector adjacent to the enemy (towards our base)
			// or the nearest unclaimed sector
			// TODO: should take current platoon position into account
			auto val = mSectorMap.getValue(i, j);
			if(val & SECTOR_PLANNED_ATTACK) {
				continue;
			}

			if(val & SECTOR_ENEMY_PRESENCE) {
				auto adj = mSectorMap.getAdjacentSectors({i, j});
				for(auto& s : adj) {
					float dist = mSectorMap.getDistanceBetween(homesec, s);
					if(dist < maxdist) {
						threatsec = s;
						maxdist = dist;
					}
				}
			} else if(val == 0) {
				float dist = mSectorMap.getDistanceBetween(homesec, {i, j});
				if(dist < maxdist) {
					threatsec = {i, j};
					maxdist = dist;
				}
			}
		}
	}

	if(maxdist != INT_MAX) {
		return mSectorMap.getSector(threatsec);
	} else {
		auto enemyvec = mWorld->getHomeBasePosition(!mSoldier->getSide()->isFirst());
		return mSectorMap.getSector(enemyvec);
	}
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
				InfoChannel::getInstance()->say(mSoldier, "Attacking an enemy");
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

