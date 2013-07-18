#include <string.h>
#include <stdexcept>
#include <float.h>

#include "World.h"
#include "SensorySystem.h"
#include "DebugOutput.h"
#include "InfoChannel.h"

#include "common/Random.h"

using namespace Common;

namespace Brigades {

Tree::Tree(const Vector3& pos, float radius)
	: Obstacle(radius)
{
	mPosition = pos;
}


Bullet::Bullet(const SoldierPtr shooter, const Vector3& pos, const Vector3& vel, float timeleft)
	: mShooter(shooter),
	mTimer(timeleft)
{
	mTimer.rewind();
	mPosition = pos;
	mVelocity = vel + shooter->getVelocity();
	mWeapon = shooter->getCurrentWeapon();
	assert(mWeapon);

	// build cache of trees that may be in the flight line for collision detection
	mObstacleCache = shooter->getWorld()->getTreesAt(mPosition + mVelocity * timeleft * 0.5f,
			5.0f + mVelocity.length() * timeleft * 0.6f);
	Vector3 endpos = mPosition + mVelocity * timeleft * 1.2f;

	for(auto it = mObstacleCache.begin(); it != mObstacleCache.end(); ) {
		if(!Math::segmentCircleIntersect(mPosition, endpos,
					(*it)->getPosition(), (*it)->getRadius() * 2.0f)) {
			it = mObstacleCache.erase(it);
		} else {
			++it;
		}
	}
}

const std::vector<Tree*>& Bullet::getObstacleCache() const
{
	return mObstacleCache;
}

void Bullet::update(float time)
{
	if(!isAlive())
		return;

	Entity::update(time);
	mTimer.doCountdown(time);
	mTimer.check();
}

bool Bullet::isAlive() const
{
	return mTimer.running();
}

SoldierPtr Bullet::getShooter() const
{
	return mShooter;
}

const WeaponPtr Bullet::getWeapon() const
{
	return mWeapon;
}

float Bullet::getOriginalSpeed() const
{
	return mWeapon->getVelocity();
}

float Bullet::getFlyTime() const
{
	return mTimer.getMaxTime() - mTimer.timeLeft();
}

Foxhole::Foxhole(const WorldPtr world, const Common::Vector3& pos)
	: mWorld(world),
	mPosition(pos),
	mDepth(0.0f)
{
}

void Foxhole::deepen(float d)
{
	mDepth += d;
	if(mDepth > 1.0f)
		mDepth = 1.0f;
}

float Foxhole::getDepth() const
{
	return mDepth;
}

const Common::Vector3& Foxhole::getPosition() const
{
	return mPosition;
}

int Timestamp::secondDifferenceTo(const Timestamp& ts) const
{
	int sd = Second - ts.Second;
	sd += (Minute - ts.Minute) * 60;
	sd += (Hour - ts.Hour) * 3600;
	sd += (Day - ts.Day) * 86400;
	return sd;
}

void Timestamp::addMilliseconds(unsigned int ms)
{
	Millisecond += ms;
	Second += Millisecond / 1000;
	Millisecond = Millisecond % 1000;
	Minute += Second / 60;
	Second = Second % 60;
	Hour += Minute / 60;
	Minute = Minute % 60;
	Day += Hour / 24;
	Hour = Hour % 24;
}


const float World::TimeCoefficient = 60.0f;

World::World(float width, float height, float visibility, 
		float sounddistance, UnitSize unitsize, bool dictator, Armory& armory)
	: mWidth(width),
	mHeight(height),
	mMaxSoldiers(1024),
	mSoldierCSP(mWidth, mHeight, mWidth / 32, mHeight / 32, mMaxSoldiers),
	mTrees(AABB(Vector2(0, 0), Vector2(mWidth * 0.5f, mHeight * 0.5f))),
	mFoxholes(AABB(Vector2(0, 0), Vector2(mWidth * 0.5f, mHeight * 0.5f))),
	mMaxVisibility(visibility),
	mSoundDistance(sounddistance),
	mTeamWon(-1),
	mSoldiersAtStart(0),
	mWinTimer(1.0f),
	mReapTimer(30.0f),
	mSquareSide(64),
	mArmory(armory),
	mUnitSize(unitsize),
	mDictator(dictator),
	mReinforcementTimer{3600.0f / TimeCoefficient, 3600.0f / TimeCoefficient}
{
	memset(mSoldiersAlive, 0, sizeof(mSoldiersAlive));

	updateVisibility();

	for(int i = 0; i < NUM_SIDES; i++) {
		mSides[i] = SidePtr(new Side(i == 0));
	}

	setHomeBasePositions();

	mTime.Hour = 6;
}

void World::create()
{
	addWalls();
	addTrees();
	setupSides();
}

// accessors
std::vector<Tree*> World::getTreesAt(const Vector3& v, float radius) const
{
	return mTrees.query(AABB(Vector2(v.x, v.y), Vector2(radius, radius)));
}

std::vector<SoldierPtr> World::getSoldiersAt(const Vector3& v, float radius)
{
	std::vector<SoldierPtr> res;
	for(auto s = mSoldierCSP.queryBegin(Vector2(v.x, v.y), radius);
			!mSoldierCSP.queryEnd();
			s = mSoldierCSP.queryNext()) {
		res.push_back(s);
	}

	return res;
}

std::list<BulletPtr> World::getBulletsAt(const Common::Vector3& v, float radius) const
{
	/* TODO */
	return mBullets;
}

std::vector<Foxhole*> World::getFoxholesAt(const Common::Vector3& v, float radius) const
{
	return mFoxholes.query(AABB(Vector2(v.x, v.y), Vector2(radius, radius)));
}

float World::getWidth() const
{
	return mWidth;
}

float World::getHeight() const
{
	return mHeight;
}

SidePtr World::getSide(bool first) const
{
	return mSides[first ? 0 : 1];
}

std::vector<WallPtr> World::getWallsAt(const Common::Vector3& v, float radius) const
{
	/* TODO */
	return mWalls;
}

std::vector<Foxhole*> World::getFoxholesInFOV(const SoldierPtr p)
{
	std::vector<Foxhole*> nearbytgts = getFoxholesAt(p->getPosition(), mVisibility);
	std::vector<Tree*> nearbytrees = getTreesAt(p->getPosition(), mVisibility);
	std::vector<Foxhole*> ret;

	for(auto s : nearbytgts) {
		float distToMe = s->getPosition().distance(p->getPosition());

		if(distToMe < 2.0f) {
			ret.push_back(s);
			continue;
		}

		if(distToMe > mVisibility * 4.0f) {
			continue;
		}

		bool treeblocks = false;
		for(auto t : nearbytrees) {
			if(Math::segmentCircleIntersect(p->getPosition(), s->getPosition(),
						t->getPosition(), t->getRadius())) {
				treeblocks = true;
				break;
			}
		}
		if(treeblocks) {
			continue;
		}

		ret.push_back(s);
	}

	return ret;
}

std::vector<SoldierPtr> World::getSoldiersInFOV(const SoldierPtr p)
{
	std::vector<SoldierPtr> nearbysoldiers = getSoldiersAt(p->getPosition(), mVisibility);
	std::vector<Tree*> nearbytrees;
	bool haveTrees = false;
	std::vector<SoldierPtr> ret;

	for(auto s : nearbysoldiers) {
		if(s.get() == p.get()) {
			ret.push_back(s);
			continue;
		}

		float distToMe = Entity::distanceBetween(*p, *s);

		if(distToMe < 2.0f) {
			// "see" anyone just behind my back
			ret.push_back(s);
			continue;
		}

		if(distToMe < mVisibility * 2.0f * s->getRadius() && s->getSideNum() == p->getSideNum()) {
			// "see" any nearby friendly soldiers
			ret.push_back(s);
			continue;
		}

		if(distToMe > mVisibility * 2.0f * s->getRadius()) {
			continue;
		}

		float dot = p->getHeadingVector().normalized().dot((s->getPosition() - p->getPosition()).normalized());
		if(acos(dot) > p->getFOV() * 0.5f) {
			continue;
		}

		bool treeblocks = false;
		if(!haveTrees) {
			nearbytrees = getTreesAt(p->getPosition(), mVisibility);
			haveTrees = true;
		}

		for(auto t : nearbytrees) {
			if(Math::segmentCircleIntersect(p->getPosition(), s->getPosition(),
						t->getPosition(), t->getRadius())) {
				treeblocks = true;
				break;
			}
		}
		if(treeblocks) {
			continue;
		}

		ret.push_back(s);
	}

	return ret;
}

int World::teamWon() const
{
	return mTeamWon;
}

int World::soldiersAlive(int t) const
{
	if(t >= NUM_SIDES || t < 0) {
		throw std::runtime_error("World::soldiersAlive: invalid parameter");
	}
	return mSoldiersAlive[t];
}

const TriggerSystem& World::getTriggerSystem() const
{
	return mTriggerSystem;
}

const Vector3& World::getHomeBasePosition(bool first) const
{
	return mHomeBasePositions[first ? 0 : 1];
}

float World::getVisibility() const
{
	return mVisibility;
}

float World::getVisibilityFactor() const
{
	return mVisibility / mMaxVisibility;
}

float World::getShootSoundHearingDistance() const
{
	return mSoundDistance;
}

Armory& World::getArmory() const
{
	return mArmory;
}

Common::Rectangle World::getArea() const
{
	return Rectangle(-mWidth * 0.5f, -mHeight * 0.5f, mWidth, mHeight);
}


// modifiers
void World::update(float time)
{
	for(auto sit : mSoldierMap) {
		auto s = sit.second;
		if(!s->isDead()) {
			auto oldpos = s->getPosition();
			assert(!isnan(s->getPosition().x));
			s->update(time);

			assert(!isnan(s->getPosition().x));
			checkSoldierPosition(s);
			assert(!isnan(s->getPosition().x));
			mSoldierCSP.update(s, Vector2(oldpos.x, oldpos.y), Vector2(s->getPosition().x, s->getPosition().y));
		}
	}

	auto bit = mBullets.begin();
	while(bit != mBullets.end()) {
		bool erase = false;
		auto soldiers = getSoldiersAt((*bit)->getPosition(), (*bit)->getVelocity().length());
		for(auto s : soldiers) {
			if(mTeamWon != -1)
				continue;

			if(s->getSideNum() == (*bit)->getShooter()->getSideNum())
				continue;

			if(s->isDead())
				continue;

			float soldierWidth = s->getRadius();
			auto foxhole = getFoxholeAt(s->getPosition());
			if(foxhole)
				soldierWidth = soldierWidth * (1.0f - foxhole->getDepth() * 0.8f);

			if(Math::segmentCircleIntersect((*bit)->getPosition(),
						(*bit)->getPosition() + (*bit)->getVelocity() * time,
						s->getPosition(), soldierWidth)) {
				s->reduceHealth(s->damageFactorFromWeapon((*bit)->getWeapon()));
				if(s->getHealth() <= 0.0f) {
					killSoldier(s);
					if(s->isDictator()) {
						mTeamWon = (*bit)->getShooter()->getSideNum();
					}
				}
				erase = true;
				break;
			}
		}

		// have bullets pass through trees for the first 100ms of flight
		if((*bit)->getFlyTime() > 0.1f) {
			for(auto t : (*bit)->getObstacleCache()) {
				if(Math::segmentCircleIntersect((*bit)->getPosition(),
							(*bit)->getPosition() + (*bit)->getVelocity() * time,
							t->getPosition(), t->getRadius())) {
					(*bit)->setVelocity((*bit)->getVelocity() * 0.8f);
					float orig = (*bit)->getOriginalSpeed();
					orig *= 0.5f;
					orig = orig * orig;
					if((*bit)->getVelocity().length2() < orig) {
						erase = true;
					}
				}
			}
		}

		if(!erase) {
			(*bit)->update(time);
			if(!(*bit)->isAlive()) {
				erase = true;
			}
		}

		if(erase) {
			bit = mBullets.erase(bit);
		} else {
			++bit;
		}
	}

	if(mWinTimer.check(time)) {
		checkForWin();
	}
	if(mReapTimer.check(time)) {
		reapDeadSoldiers();
	}

	updateTriggerSystem(time);

	if(mUnitSize > UnitSize::Squad) {
		for(unsigned int i = 0; i < NUM_SIDES; i++) {
			if(mRootLeader[i]->isDead())
				continue;

			auto& c = mReinforcementTimer[i];
			c.doCountdown(time);
			if(c.check()) {
				if(mSoldiersAlive[i] * 2 < mSoldiersAtStart) {
					auto s = addUnit(UnitSize(int(mUnitSize) - 1), i, true);
					auto rootCommandees = mRootLeader[i]->getCommandees();

					if(std::find(rootCommandees.begin(), rootCommandees.end(), s) == rootCommandees.end()) {
						// new commandee for the root leader
						mRootLeader[i]->addCommandee(s);
						mRootLeader[i]->getController()->handleReinforcement(s);
					}
					float oldTime = mReinforcementTimer[i].getMaxTime();
					float newTime = oldTime + 3600.0f / TimeCoefficient;
					mReinforcementTimer[i] = Common::Countdown(newTime);

					char buf[128];
					snprintf(buf, 127, "The %s team got reinforcement",
							i == 0 ? "Red" : "Blue");
					buf[127] = 0;
					InfoChannel::getInstance()->addMessage(SoldierPtr(), Common::Color::White, buf);
				} else {
					c.rewind();
				}
			}
		}
	}

	mTime.addMilliseconds(time * TimeCoefficient * 1000);
	updateVisibility();
}

void World::updateVisibility()
{
	// visibility calculation based on time of day
	float t = - 2.0f * cos(PI / 12 * (mTime.Hour + mTime.Minute / 60.0f));
	t = Common::clamp(0.1f, t, 1.0f);
	mVisibility = t * mMaxVisibility;
}

const Timestamp& World::getCurrentTime() const
{
	return mTime;
}

std::string World::getCurrentTimeAsString() const
{
	char buf[128];
	auto ts = getCurrentTime();
	snprintf(buf, 128, "Day %d %02d:%02d", ts.Day, ts.Hour, ts.Minute);
	buf[127] = 0;
	return std::string(buf);
}

float World::getTimeCoefficient() const
{
	return TimeCoefficient;
}

void World::addBullet(const WeaponPtr w, const SoldierPtr s, const Vector3& dir)
{
	float time = w->getRange() / w->getVelocity();
	mBullets.push_back(BulletPtr(new Bullet(s,
				s->getPosition(),
				dir.normalized() * w->getVelocity(),
				time)));
	auto nearbySoldiers = getSoldiersAt(s->getPosition(), getShootSoundHearingDistance());
	SoundTrigger trigger(s, getShootSoundHearingDistance());
	mTriggerSystem.tryOneShotTrigger(trigger, nearbySoldiers);
}

void World::dig(float time, const Common::Vector3& pos)
{
	Foxhole* foxhole = getFoxholeAt(pos);
	if(!foxhole) {
		// we're leaking the foxholes for now.
		foxhole = new Foxhole(shared_from_this(), pos);
		bool ret = mFoxholes.insert(foxhole, Vector2(pos.x, pos.y));
		if(!ret) {
			std::cerr << "Error: couldn't add foxhole at " << pos << "\n";
			assert(0);
			return;
		}
	}
	// 4 hours game time for completion
	foxhole->deepen(time * TimeCoefficient / 14400.0f);
}

void World::createMovementSound(const SoldierPtr s)
{
	float dist = getShootSoundHearingDistance() * 0.3f;
	if(s->getWarriorType() == WarriorType::Vehicle)
		dist *= 3.0f;
	auto nearbySoldiers = getSoldiersAt(s->getPosition(), dist);
	SoundTrigger trigger(s, dist);
	mTriggerSystem.tryOneShotTrigger(trigger, nearbySoldiers);
}

Foxhole* World::getFoxholeAt(const Common::Vector3& pos)
{
	Foxhole* p = nullptr;
	float mindist2 = FLT_MAX;
	for(auto f : mFoxholes.query(AABB(Vector2(pos.x, pos.y), Vector2(1.5f, 1.5f)))) {
		float d2 = f->getPosition().distance2(pos);
		if(d2 < mindist2) {
			mindist2 = d2;
			p = f;
		}
	}
	return p;
}

// reuseLeader is a hint to try to avoid creating a new leader if possible.
SoldierPtr World::addUnit(UnitSize u, unsigned int side, bool reuseLeader)
{
	switch(u) {
		case UnitSize::Squad:
			return addSquad(side, reuseLeader);

		case UnitSize::Platoon:
			return addPlatoon(side, reuseLeader);

		case UnitSize::Company:
			return addCompany(side, reuseLeader);
	}
}

void World::setupSides()
{
	for(int i = 0; i < NUM_SIDES; i++) {
		mRootLeader[i] = addUnit(mUnitSize, i);

		if(mDictator) {
			addDictator(i);
		}
	}
	mSoldiersAtStart = mSoldiersAlive[0];
}

SoldierPtr World::addSoldier(bool first, SoldierRank rank, WarriorType wt, bool dictator)
{
	if(mSoldierMap.size() >= mMaxSoldiers) {
		assert(0);
		throw std::runtime_error("Too many soldiers in the world");
	}

	SoldierPtr s = SoldierPtr(new Soldier(shared_from_this(), first, rank, wt));
	s->init();

	if(dictator) {
		s->setDictator(true);
		s->clearWeapons();
		s->addWeapon(mArmory.getPistol());
	}
	else {
		mSoldiersAlive[first ? 0 : 1]++;
	}

	s->setPosition(getHomeBasePosition(first));
	mSoldierCSP.add(s, Vector2(s->getPosition().x, s->getPosition().y));
	mSoldierMap.insert(std::make_pair(s->getID(), s));
	return s;
}

void World::addTrees()
{
	int numXSquares = mWidth / mSquareSide;
	int numYSquares = mHeight / mSquareSide;

	for(int k = -numYSquares / 2; k < numYSquares / 2; k++) {
		for(int j = -numXSquares / 2; j < numXSquares / 2; j++) {
			if((k == -numYSquares / 2 && j == -numXSquares / 2) ||
				       (k == numYSquares / 2 - 1 && j == numXSquares / 2 -1))
				continue;

			int treefactor = Random::uniform() * 10;
			for(int i = 0; i < treefactor; i++) {
				float x = Random::uniform();
				float y = Random::uniform();
				float r = Random::uniform();

				const float maxRadius = 8.0f;

				x *= mSquareSide;
				y *= mSquareSide;
				x += j * mSquareSide;
				y += k * mSquareSide;
				r = Common::clamp(2.0f, r * maxRadius, maxRadius);

				bool tooclose = false;
				for(auto t : mTrees.query(AABB(Vector2(x, y), Vector2(maxRadius * 2.0f, maxRadius * 2.0f)))) {
					float maxdist = r + t->getRadius();
					if(Vector3(x, y, 0.0f).distance2(t->getPosition()) <
							maxdist * maxdist) {
						tooclose = true;
						break;
					}
				}
				if(tooclose) {
					continue;
				}

				// we're leaking the trees for now.
				Tree* tree = new Tree(Vector3(x, y, 0), r);
				bool ret = mTrees.insert(tree, Vector2(x, y));
				if(!ret) {
					std::cout << "Error: couldn't add tree at " << x << ", " << y << "\n";
					assert(0);
				}
			}
		}
	}
	std::cout << "Added " << mTrees.size() << " trees.\n";
}

void World::addWalls()
{
	const float wallDistance = 0.1f;
	Vector3 a(-mWidth * 0.5f + wallDistance, -mHeight * 0.5f + wallDistance, 0.0f);
	Vector3 b( mWidth * 0.5f - wallDistance, -mHeight * 0.5f + wallDistance, 0.0f);
	Vector3 c(-mWidth * 0.5f + wallDistance,  mHeight * 0.5f - wallDistance, 0.0f);
	Vector3 d( mWidth * 0.5f - wallDistance,  mHeight * 0.5f - wallDistance, 0.0f);

	mWalls.push_back(WallPtr(new Wall(a, b)));
	mWalls.push_back(WallPtr(new Wall(a, c)));
	mWalls.push_back(WallPtr(new Wall(b, d)));
	mWalls.push_back(WallPtr(new Wall(c, d)));
}

void World::checkSoldierPosition(SoldierPtr s)
{
	for(auto t : getTreesAt(s->getPosition(), s->getRadius() * 2.0f)) {
		Vector3 diff = s->getPosition() - t->getPosition();
		float dist2 = diff.length2();
		float mindist = t->getRadius() + s->getRadius();
		if(dist2 < mindist * mindist) {
			if(dist2 == 0.0f)
				diff = Vector3(1, 0, 0);
			Vector3 shouldpos = t->getPosition() + diff.normalized() * mindist;
			s->setPosition(shouldpos);
			s->setVelocity(diff * 0.3f);
		}
	}

	if(fabs(s->getPosition().x) > mWidth * 0.5f) {
		float nx = mWidth * 0.5f - 1.0f;
		Vector3 p = s->getPosition();
		if(s->getPosition().x < 0.0f) {
			p.x = -nx;
		} else {
			p.x = nx;
		}
		s->setPosition(p);
		s->setVelocity(Vector3());
	}

	if(fabs(s->getPosition().y) > mHeight * 0.5f) {
		float ny = mHeight * 0.5f - 1.0f;
		Vector3 p = s->getPosition();
		if(s->getPosition().y < 0.0f) {
			p.y = -ny;
		} else {
			p.y = ny;
		}
		s->setPosition(p);
		s->setVelocity(Vector3());
	}
}

void World::checkForWin()
{
	if(mTeamWon != -1)
		return;

	int winningTeam = -1;
	for(int i = 0; i < NUM_SIDES; i++) {
		if(mSoldiersAlive[i] != 0) {
			if(winningTeam == -1) {
				winningTeam = i;
			}
			else {
				return;
			}
		}
	}

	if(winningTeam == -1) {
		mTeamWon = -2;
	}
	else {
		mTeamWon = winningTeam;
	}
}

void World::killSoldier(SoldierPtr s)
{
	if(s->isDead())
		return;

	s->die();
	if(s->getWarriorType() == WarriorType::Soldier) {
		for(auto w : s->getWeapons()) {
			Vector3 offset = Vector3(1.0f * Random::clamped(), 1.0f * Random::clamped(), 0.0f);
			mTriggerSystem.add(WeaponPickupTriggerPtr(new WeaponPickupTrigger(w, s->getPosition() + offset)));
		}
	}

	if(!s->isDictator()) {
		assert(s->getSideNum() < NUM_SIDES);
		assert(mSoldiersAlive[s->getSideNum()] > 0);
		mSoldiersAlive[s->getSideNum()]--;
	}
}

void World::updateTriggerSystem(float time)
{
	std::vector<SoldierPtr> soldiers;
	for(auto s : mSoldierMap) {
		soldiers.push_back(s.second);
	}
	mTriggerSystem.update(soldiers, time);
}

SoldierPtr World::addCompany(int side, bool reuseLeader)
{
	SoldierPtr companyleader;

	companyleader = addSoldier(side == 0, SoldierRank::Captain, WarriorType::Soldier, false);

	for(int k = 0; k < 3; k++) {
		auto s = addPlatoon(side, reuseLeader);
		assert(s);
		companyleader->addCommandee(s);
	}

	return companyleader;
}

SoldierPtr World::addPlatoon(int side, bool reuseLeader)
{
	SoldierPtr platoonleader;
	bool reusing = false;

	if(reuseLeader) {
		auto root = mRootLeader[side];
		if(root && root->getRank() == SoldierRank::Captain) {
			int least = INT_MAX;
			for(auto& n : root->getCommandees()) {
				if(n->getCommandees().size() < least) {
					platoonleader = n;
					least = n->getCommandees().size();
					reusing = true;
				}
			}
		}
	}

	if(!platoonleader) {
		platoonleader = addSoldier(side == 0, SoldierRank::Lieutenant, WarriorType::Soldier, false);
	}

	for(int k = 0; k < 3; k++) {
		auto s = addSquad(side, reuseLeader);
		assert(s);
		platoonleader->addCommandee(s);
		if(reusing) {
			// new commandee for an own leader - notify controller
			platoonleader->getController()->handleReinforcement(s);
		}
	}

	return platoonleader;
}

SoldierPtr World::addSquad(int side, bool reuseLeader)
{
	SoldierPtr squadleader;
	for(int j = 0; j < 8; j++) {
		WarriorType wt = WarriorType::Soldier;
		if(j == 7)
			wt = WarriorType::Vehicle;

		auto s = addSoldier(side == 0, j == 0 ? SoldierRank::Sergeant : SoldierRank::Private, wt, false);
		if(j == 0) {
			squadleader = s;
		}
		else {
			if(j == 2) {
				s->clearWeapons();
				s->addWeapon(mArmory.getMachineGun());
			}
			else if(j == 3 || j == 4 || j == 5) {
				s->addWeapon(mArmory.getBazooka());
			}
			squadleader->addCommandee(s);
		}
	}
	return squadleader;
}

void World::addDictator(int side)
{
	addSoldier(side == 0, SoldierRank::Private, WarriorType::Soldier, true);
}

void World::setHomeBasePositions()
{
	float x = mWidth * 0.5f - mSquareSide * 0.5f;
	float y = mHeight * 0.5f - mSquareSide * 0.5f;
	mHomeBasePositions[0] = Vector3(-x, -y, 0);
	mHomeBasePositions[1] = Vector3(x, y, 0);
}

void World::reapDeadSoldiers()
{
	auto it = mSoldierMap.begin();
	while(it != mSoldierMap.end()) {
		if(it->second->isDead()) {
			mSoldierCSP.remove(it->second, Vector2(it->second->getPosition().x, it->second->getPosition().y));
			it = mSoldierMap.erase(it);
		} else {
			++it;
		}
	}
}

}


