#include <string.h>
#include <stdexcept>
#include <float.h>

#include "World.h"
#include "SensorySystem.h"
#include "DebugOutput.h"
#include "InfoChannel.h"

#include "common/Random.h"

#include "ai/SoldierController.h"

using namespace Common;

namespace Brigades {

Tree::Tree(const Vector3& pos, float radius)
	: Obstacle(radius)
{
	mPosition = pos;
}

Side::Side(bool first)
	: mFirst(first)
{
}

bool Side::isFirst() const
{
	return mFirst;
}

int Side::getSideNum() const
{
	return mFirst ? 0 : 1;
}

AttackOrder::AttackOrder(const Common::Vector3& p, const Common::Vector3& d, float width)
	: CenterPoint(p)
{
	Vector3 v = d - p;
	v.normalize();
	v *= width;
	DefenseLineToRight.x = v.y;
	DefenseLineToRight.y = -v.x;
}

SoldierController::SoldierController()
	: mLeaderStatusTimer(1.0f)
{
}

SoldierController::SoldierController(boost::shared_ptr<Soldier> s)
	: mWorld(s->getWorld()),
	mSoldier(s),
	mSteering(boost::shared_ptr<Steering>(new Steering(*s))),
	mLeaderStatusTimer(1.0f)
{
}

void SoldierController::setSoldier(boost::shared_ptr<Soldier> s)
{
	if(mSoldier) {
		mSoldier->setAIController();
	}
	mSoldier = s;
	mWorld = mSoldier->getWorld();
	mSteering = boost::shared_ptr<Steering>(new Steering(*mSoldier));
	mSoldier->setController(shared_from_this());
}

Vector3 SoldierController::defaultMovement(float time)
{
	std::vector<boost::shared_ptr<Tree>> trees = mWorld->getTreesAt(mSoldier->getPosition(),
			mSoldier->getRadius() * 2.0f + mSoldier->getVelocity().length());
	std::vector<Obstacle*> obstacles(trees.size());
	for(unsigned int i = 0; i < trees.size(); i++)
		obstacles[i] = trees[i].get();

	std::vector<WallPtr> wallptrs = mWorld->getWallsAt(mSoldier->getPosition(), mSoldier->getVelocity().length());
	std::vector<Wall*> walls(wallptrs.size());
	for(unsigned int i = 0; i < wallptrs.size(); i++)
		walls[i] = wallptrs[i].get();

	Vector3 obs = mSteering->obstacleAvoidance(obstacles) * 100.0f;
	Vector3 wal = mSteering->wallAvoidance(walls) * 100.0f;

	Vector3 tot;
	mSteering->accumulate(tot, wal);
	mSteering->accumulate(tot, obs);

	return tot;
}

void SoldierController::moveTo(const Common::Vector3& dir, float time, bool autorotate)
{
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
}

void SoldierController::turnTo(const Common::Vector3& dir)
{
	mSoldier->setXYRotation(atan2(dir.y, dir.x));
}

void SoldierController::turnBy(float rad)
{
	mSoldier->addXYRotation(rad);
}

void SoldierController::setVelocityToHeading()
{
	mSoldier->setVelocityToHeading();
}

boost::shared_ptr<Common::Steering> SoldierController::getSteering()
{
	return mSteering;
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
				mSoldier->setDefending();
				return true;
			}
			break;

		default:
			break;
	}

	return false;
}

Soldier::Soldier(boost::shared_ptr<World> w, bool firstside, SoldierRank rank, WarriorType wt)
	: Common::Vehicle(0.5f, 10.0f, 100.0f),
	mWorld(w),
	mSide(w->getSide(firstside)),
	mID(getNextID()),
	mFOV(PI),
	mAlive(true),
	mCurrentWeaponIndex(0),
	mRank(rank),
	mWarriorType(wt),
	mHealth(1.0f),
	mDictator(false),
	mAttacking(false),
	mEnemyContact(false),
	mEnemyContactTimer(1.0f)
{
	mName = generateName();
	if(wt == WarriorType::Vehicle) {
		mRadius = 3.5f;
		mMaxSpeed = 30.0f;
		mMaxAcceleration = 10.0f;
		addWeapon(mWorld->getArmory().getAutomaticCannon());
		addWeapon(mWorld->getArmory().getMachineGun());
		mFOV = TWO_PI;
	} else {
		addWeapon(mWorld->getArmory().getAssaultRifle());
	}
}

void Soldier::init()
{
	setAIController();
	mSensorySystem = SensorySystemPtr(new SensorySystem(shared_from_this()));
}

SidePtr Soldier::getSide() const
{
	return mSide;
}

int Soldier::getID() const
{
	return mID;
}

int Soldier::getSideNum() const
{
	return mSide->getSideNum();
}

int Soldier::getNextID()
{
	static int id = 0;
	return ++id;
}

std::string Soldier::generateName()
{
	static char n = 'A';

	std::string s(1, n);
	if(n == 'Z') {
		n = 'A';
	} else {
		n++;
	}

	return s;
}

void Soldier::update(float time)
{
	if(!time)
		return;

	mSensorySystem->update(time);
	if(mCurrentWeaponIndex < mWeapons.size())
		mWeapons[mCurrentWeaponIndex]->update(time);
	if(mController) {
		mController->act(time);
	}

	mEnemyContactTimer.doCountdown(time);
	if(mEnemyContactTimer.checkAndRewind()) {
		mEnemyContact = false;
		for(auto s : mSensorySystem->getSoldiers()) {
			if(!s->isDead() && s->getSideNum() != getSideNum() &&
					mPosition.distance2(s->getPosition()) <
					mWorld->getVisibilityFactor() *
					mWorld->getVisibilityFactor()) {
				mEnemyContact = true;
				break;
			}
		}

		if(!mEnemyContact) {
			for(auto s : mCommandees) {
				if(s->hasEnemyContact()) {
					mEnemyContact = true;
					break;
				}
			}
		}
	}

}

float Soldier::getFOV() const
{
	return mFOV;
}

void Soldier::setController(SoldierControllerPtr p)
{
	mController = p;
}

void Soldier::setAIController()
{
	setController(SoldierControllerPtr(new AI::SoldierController(shared_from_this())));
}

SoldierControllerPtr Soldier::getController()
{
	return mController;
}

void Soldier::die()
{
	mAlive = false;
}

bool Soldier::isDead() const
{
	return !mAlive;
}

void Soldier::dig(float time)
{
	mWorld->dig(time, mPosition);
}

void Soldier::clearWeapons()
{
	mWeapons.clear();
}

void Soldier::addWeapon(WeaponPtr w)
{
	mWeapons.push_back(w);
}

WeaponPtr Soldier::getCurrentWeapon()
{
	if(mCurrentWeaponIndex < mWeapons.size())
		return mWeapons[mCurrentWeaponIndex];
	else
		return WeaponPtr();
}

void Soldier::switchWeapon(unsigned int index)
{
	mCurrentWeaponIndex = index;
}

const std::vector<WeaponPtr>& Soldier::getWeapons() const
{
	return mWeapons;
}

const WorldPtr Soldier::getWorld() const
{
	return mWorld;
}

WorldPtr Soldier::getWorld()
{
	return mWorld;
}

SensorySystemPtr Soldier::getSensorySystem()
{
	return mSensorySystem;
}

void Soldier::addEvent(EventPtr e)
{
	mEvents.push_back(e);
}

std::vector<EventPtr>& Soldier::getEvents()
{
	return mEvents;
}

bool Soldier::handleEvents()
{
	bool ret = !mEvents.empty();

	for(auto e : mEvents) {
		e->handleEvent(shared_from_this());
	}

	mEvents.clear();

	return ret;
}

SoldierRank Soldier::getRank() const
{
	return mRank;
}

void Soldier::setRank(SoldierRank r)
{
	mRank = r;
}

void Soldier::addCommandee(SoldierPtr s)
{
	if(!isDead() && !s->isDead()) {
		mCommandees.push_back(s);
		s->setLeader(shared_from_this());
	}
}

void Soldier::removeCommandee(SoldierPtr s)
{
	mCommandees.remove(s);
	s->setLeader(SoldierPtr());
}

std::list<SoldierPtr>& Soldier::getCommandees()
{
	return mCommandees;
}

void Soldier::setLeader(SoldierPtr s)
{
	mLeader = s;
}

SoldierPtr Soldier::getLeader()
{
	return mLeader;
}

bool Soldier::seesSoldier(const SoldierPtr s)
{
	for(auto ss : mSensorySystem->getSoldiers()) {
		if(s == ss) {
			return true;
		}
	}

	return false;
}

void Soldier::setFormationOffset(const Vector3& v)
{
	mDefendPosition = Vector3();
	mFormationOffset = v;
}

void Soldier::setDefendPosition(const Common::Vector3& v)
{
	mFormationOffset = Vector3();
	mDefendPosition = v;
}

const Vector3& Soldier::getFormationOffset() const
{
	return mFormationOffset;
}

const Vector3& Soldier::getDefendPosition() const
{
	return mDefendPosition;
}

void Soldier::setColumnFormation(float dist)
{
	int i = 1;
	for(auto c : mCommandees) {
		float pos = i * -dist;
		c->setFormationOffset(Vector3(pos, 0.0f, 0.0f));
		i++;
	}
}

void Soldier::setLineFormation(float dist)
{
	int i = 2;
	for(auto c : mCommandees) {
		float pos = (i / 2) * dist * (i & 1 ? 1.0f : -1.0f);
		c->setFormationOffset(Vector3(0.0f, pos, 0.0f));
		i++;
	}
}

void Soldier::pruneCommandees()
{
	auto sit = mCommandees.begin();
	while(sit != mCommandees.end()) {
		if((*sit)->isDead()) {
			sit = mCommandees.erase(sit);
		}
		else {
			++sit;
		}
	}

}

WarriorType Soldier::getWarriorType() const
{
	return mWarriorType;
}

void Soldier::reduceHealth(float n)
{
	mHealth -= n;
}

float Soldier::getHealth() const
{
	return mHealth;
}

float Soldier::damageFactorFromWeapon(const WeaponPtr w) const
{
	if(getWarriorType() == WarriorType::Soldier) {
		return w->getDamageAgainstSoftTargets();
	} else {
		return w->getDamageAgainstLightArmor();
	}
}

bool Soldier::hasWeaponType(const char* wname) const
{
	for(auto w : mWeapons) {
		if(!strcmp(wname, w->getName()))
			return true;
	}
	return false;
}

void Soldier::setDictator(bool d)
{
	mDictator = d;
	if(mDictator) {
		mMaxSpeed = 0.0f;
		mMaxAcceleration = 0.0f;
	}
}

bool Soldier::isDictator() const
{
	return mDictator;
}

bool Soldier::canCommunicateWith(const SoldierPtr p) const
{
	return !isDead() && !p->isDead() && ((hasRadio() && p->hasRadio()) ||
			(Entity::distanceBetween(*this, *p) < 10.0f));
}

bool Soldier::hasRadio() const
{
	return true;
}

bool Soldier::hasEnemyContact() const
{
	return mEnemyContact;
}

const std::string& Soldier::getName() const
{
	return mName;
}

const char* Soldier::rankToString(SoldierRank r)
{
	switch(r) {
		case SoldierRank::Private:
			return "Private";
		case SoldierRank::Sergeant:
			return "Sergeant";
		case SoldierRank::Lieutenant:
			return "Lieutenant";
		case SoldierRank::Captain:
			return "Captain";
	}
}

bool Soldier::defending() const
{
	return !mAttacking;
}

void Soldier::setDefending()
{
	mAttacking = false;
}

bool Soldier::giveAttackOrder(const AttackOrder& r)
{
	// set mAttackOrder for handleAttackOrder(), then set it back in case of failure
	mAttacking = true;
	auto old = mAttackOrder;
	mAttackOrder = r;
	if(!mController->handleAttackOrder(r)) {
		std::cout << "Warning: controller couldn't handle attack order\n";
		mAttacking = false;
		mAttackOrder = old;
		return false;
	} else {
		return true;
	}
}

const AttackOrder& Soldier::getAttackOrder() const
{
	return mAttackOrder;
}

const Common::Vector3& Soldier::getCenterOfAttackArea() const
{
	return mAttackOrder.CenterPoint;
}

void Soldier::globalMessage(const char* s)
{
	char buf[256];
	snprintf(buf, 255, "%s %s: %s", rankToString(mRank), mName.c_str(), s);
	buf[255] = 0;
	InfoChannel::getInstance()->addMessage(shared_from_this(), Common::Color::White, buf);
}

bool Soldier::reportSuccessfulAttack()
{
	if(mRank >= SoldierRank::Lieutenant) {
		char buf[256];
		buf[255] = 0;
		snprintf(buf, 255, "I'm reporting a successful attack to %s %s",
				rankToString(mLeader->getRank()),
				mLeader->getName().c_str());
		globalMessage(buf);
	}
	return mLeader->successfulAttackReported(getAttackOrder());
}

bool Soldier::successfulAttackReported(const AttackOrder& r)
{
	if(!mController->handleAttackSuccess(shared_from_this(), r)) {
		std::cerr << "Warning: controller couldn't handle successful attack\n";
		return false;
	} else {
		return true;
	}
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
	mVisibilityFactor(visibility),
	mSoundDistance(sounddistance),
	mTeamWon(-1),
	mSoldiersAtStart(0),
	mWinTimer(1.0f),
	mSquareSide(64),
	mArmory(armory),
	mUnitSize(unitsize),
	mDictator(dictator),
	mReinforcementTimer{3600.0f / TimeCoefficient, 3600.0f / TimeCoefficient}
{
	memset(mSoldiersAlive, 0, sizeof(mSoldiersAlive));

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
std::vector<TreePtr> World::getTreesAt(const Vector3& v, float radius) const
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

std::vector<FoxholePtr> World::getFoxholesAt(const Common::Vector3& v, float radius) const
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

std::vector<FoxholePtr> World::getFoxholesInFOV(const SoldierPtr p)
{
	std::vector<FoxholePtr> nearbytgts = getFoxholesAt(p->getPosition(), mVisibilityFactor);
	std::vector<TreePtr> nearbytrees = getTreesAt(p->getPosition(), mVisibilityFactor);
	std::vector<FoxholePtr> ret;

	for(auto s : nearbytgts) {
		float distToMe = s->getPosition().distance(p->getPosition());

		if(distToMe < 2.0f) {
			ret.push_back(s);
			continue;
		}

		if(distToMe > mVisibilityFactor * 4.0f) {
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
	std::vector<SoldierPtr> nearbysoldiers = getSoldiersAt(p->getPosition(), mVisibilityFactor);
	std::vector<TreePtr> nearbytrees = getTreesAt(p->getPosition(), mVisibilityFactor);
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

		if(distToMe < mVisibilityFactor * 2.0f * s->getRadius() && s->getSideNum() == p->getSideNum()) {
			// "see" any nearby friendly soldiers
			ret.push_back(s);
			continue;
		}

		if(distToMe > mVisibilityFactor * 2.0f * s->getRadius()) {
			continue;
		}

		float dot = p->getHeadingVector().normalized().dot((s->getPosition() - p->getPosition()).normalized());
		if(acos(dot) > p->getFOV() * 0.5f) {
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

float World::getVisibilityFactor() const
{
	return mVisibilityFactor;
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
			for(auto t : getTreesAt((*bit)->getPosition(), (*bit)->getVelocity().length())) {
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

	updateTriggerSystem(time);

	if(mUnitSize > UnitSize::Squad) {
		for(unsigned int i = 0; i < NUM_SIDES; i++) {
			if(mRootLeader[i]->isDead())
				continue;

			auto& c = mReinforcementTimer[i];
			c.doCountdown(time);
			if(c.check()) {
				if(mSoldiersAlive[i] * 2 < mSoldiersAtStart) {
					// TODO: need to do something about all those idle lieutenants.
					auto s = addUnit(UnitSize(int(mUnitSize) - 1), i);
					mRootLeader[i]->addCommandee(s);
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

void World::addBullet(const WeaponPtr w, const SoldierPtr s, const Vector3& dir)
{
	float time = w->getRange() / w->getVelocity();
	mBullets.push_back(BulletPtr(new Bullet(s,
				s->getPosition(),
				dir.normalized() * w->getVelocity(),
				time)));
	mTriggerSystem.add(SoundTriggerPtr(new SoundTrigger(s, getShootSoundHearingDistance())));
}

void World::dig(float time, const Common::Vector3& pos)
{
	FoxholePtr foxhole = getFoxholeAt(pos);
	if(!foxhole) {
		foxhole = FoxholePtr(new Foxhole(shared_from_this(), pos));
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

FoxholePtr World::getFoxholeAt(const Common::Vector3& pos)
{
	FoxholePtr p;
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

SoldierPtr World::addUnit(UnitSize u, unsigned int side)
{
	switch(u) {
		case UnitSize::Squad:
			return addSquad(side);

		case UnitSize::Platoon:
			return addPlatoon(side);

		case UnitSize::Company:
			return addCompany(side);
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
				r = Common::clamp(3.0f, r * maxRadius, maxRadius);

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

				TreePtr tree(new Tree(Vector3(x, y, 0), r));
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

SoldierPtr World::addCompany(int side)
{
	SoldierPtr companyleader = addSoldier(side == 0, SoldierRank::Captain, WarriorType::Soldier, false);
	for(int k = 0; k < 3; k++) {
		auto s = addPlatoon(side);
		assert(s);
		companyleader->addCommandee(s);
	}

	return companyleader;
}

SoldierPtr World::addPlatoon(int side)
{
	SoldierPtr platoonleader = addSoldier(side == 0, SoldierRank::Lieutenant, WarriorType::Soldier, false);
	for(int k = 0; k < 3; k++) {
		auto s = addSquad(side);
		assert(s);
		platoonleader->addCommandee(s);
	}

	return platoonleader;
}

SoldierPtr World::addSquad(int side)
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

}


