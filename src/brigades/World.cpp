#include <string.h>
#include <stdexcept>

#include "World.h"
#include "SensorySystem.h"

#include "common/Random.h"

#include "ai/SoldierController.h"

using namespace Common;

namespace Brigades {

Tree::Tree(const Vector3& pos, float radius)
	: Obstacle(radius)
{
	mPosition = pos;
}

Weapon::Weapon(float range, float velocity, float loadtime,
		float softd,
		float lightd,
		float heavyd)
	: mRange(range),
	mVelocity(velocity),
	mLoadTime(loadtime),
	mSoftDamage(softd),
	mLightArmorDamage(lightd),
	mHeavyArmorDamage(heavyd)
{
}

void Weapon::update(float time)
{
	mLoadTime.doCountdown(time);
	mLoadTime.check();
}

bool Weapon::canShoot() const
{
	return !mLoadTime.running();
}

float Weapon::getRange() const
{
	return mRange;
}

float Weapon::getVelocity() const
{
	return mVelocity;
}

float Weapon::getLoadTime() const
{
	return mLoadTime.getMaxTime();
}

void Weapon::shoot(WorldPtr w, const SoldierPtr s, const Vector3& dir)
{
	if(!canShoot())
		return;

	w->addBullet(shared_from_this(), s, dir);
	mLoadTime.rewind();
}

float Weapon::getDamageAgainstSoftTargets() const
{
	return mSoftDamage;
}

float Weapon::getDamageAgainstLightArmor() const
{
	return mLightArmorDamage;
}

float Weapon::getDamageAgainstHeavyArmor() const
{
	return mHeavyArmorDamage;
}

/* NOTE: increasing bullet speed is an excellent way to
 * make the game harder. */
AssaultRifle::AssaultRifle()
	: Weapon(25.0f, 20.0f, 0.1f)
{
}

const char* AssaultRifle::getName() const
{
	return "Assault Rifle";
}

MachineGun::MachineGun()
	: Weapon(35.0f, 20.0f, 0.04f)
{
}

const char* MachineGun::getName() const
{
	return "Machine Gun";
}

AutomaticCannon::AutomaticCannon()
	: Weapon(40.0f, 20.0f, 0.4f, 1.0f, 1.0f, 0.0f)
{
}

const char* AutomaticCannon::getName() const
{
	return "Automatic Cannon";
}

Bazooka::Bazooka()
	: Weapon(30.0f, 16.0f, 4.0f, 1.0f, 1.0f, 0.0f)
{
}

const char* Bazooka::getName() const
{
	return "Bazooka";
}

Pistol::Pistol()
	: Weapon(15.0f, 18.0f, 0.5f)
{
}

const char* Pistol::getName() const
{
	return "Pistol";
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

SoldierController::SoldierController()
{
}

SoldierController::SoldierController(boost::shared_ptr<Soldier> s)
	: mWorld(s->getWorld()),
	mSoldier(s),
	mSteering(boost::shared_ptr<Steering>(new Steering(*s)))
{
}

void SoldierController::setSoldier(boost::shared_ptr<Soldier> s)
{
	mSoldier = s;
	mWorld = mSoldier->getWorld();
	mSteering = boost::shared_ptr<Steering>(new Steering(*mSoldier));
	mSoldier->setController(shared_from_this());
}

Vector3 SoldierController::defaultMovement(float time)
{
	std::vector<boost::shared_ptr<Tree>> trees = mWorld->getTreesAt(mSoldier->getPosition(), mSoldier->getVelocity().length());
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
	if(dir.null() && !mSoldier->getVelocity().null()) {
		mSoldier->setAcceleration(mSoldier->getVelocity() * -10.0f);
	}
	else {
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
	mHealth(1.0f)
{
	if(wt == WarriorType::Vehicle) {
		mRadius = 3.5f;
		mMaxSpeed = 30.0f;
		mMaxAcceleration = 10.0f;
		addWeapon(WeaponPtr(new AutomaticCannon()));
		addWeapon(WeaponPtr(new MachineGun()));
		mFOV = TWO_PI;
	} else {
		addWeapon(WeaponPtr(new AssaultRifle()));
	}
}

void Soldier::init()
{
	setController(SoldierControllerPtr(new AI::SoldierController(shared_from_this())));
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
}

float Soldier::getFOV() const
{
	return mFOV;
}

void Soldier::setController(SoldierControllerPtr p)
{
	mController = p;
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

void Soldier::addCommandee(SoldierPtr s)
{
	if(!s->isDead()) {
		mCommandees.push_back(s);
		s->setLeader(shared_from_this());
	}
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

void Soldier::setFormationOffset(const Vector3& v)
{
	mFormationOffset = v;
}

const Vector3& Soldier::getFormationOffset() const
{
	return mFormationOffset;
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

World::World()
	: mWidth(256.0f),
	mHeight(256.0f),
	mVisibility(40.0f),
	mTeamWon(-1),
	mWinTimer(1.0f)
{
	memset(mSoldiersAlive, 0, sizeof(mSoldiersAlive));

	for(int i = 0; i < NUM_SIDES; i++) {
		mSides[i] = SidePtr(new Side(i == 0));
	}
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
	/* TODO */
	return mTrees;
}

std::vector<SoldierPtr> World::getSoldiersAt(const Vector3& v, float radius) const
{
	/* TODO */
	std::vector<SoldierPtr> ret;

	for(auto s : mSoldiers) {
		ret.push_back(s.second);
	}

	return ret;
}

std::list<BulletPtr> World::getBulletsAt(const Common::Vector3& v, float radius) const
{
	/* TODO */
	return mBullets;
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

std::vector<SoldierPtr> World::getSoldiersInFOV(const SoldierPtr p) const
{
	std::vector<SoldierPtr> nearbysoldiers = getSoldiersAt(p->getPosition(), mVisibility);
	std::vector<TreePtr> nearbytrees = getTreesAt(p->getPosition(), mVisibility);
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

		if(distToMe < mVisibility && s->getSideNum() == p->getSideNum()) {
			// "see" any nearby friendly soldiers
			ret.push_back(s);
			continue;
		}

		if(distToMe > mVisibility) {
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


// modifiers
void World::update(float time)
{
	for(auto s : mSoldiers) {
		if(!s.second->isDead())
			s.second->update(time);

		checkSoldierPosition(s.second);
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

			if(Math::segmentCircleIntersect((*bit)->getPosition(),
						(*bit)->getPosition() + (*bit)->getVelocity() * time,
						s->getPosition(), s->getRadius())) {
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
}

void World::addBullet(const WeaponPtr w, const SoldierPtr s, const Vector3& dir)
{
	float time = w->getRange() / w->getVelocity();
	mBullets.push_back(BulletPtr(new Bullet(s,
				s->getPosition(),
				dir.normalized() * w->getVelocity(),
				time)));
	mTriggerSystem.add(SoundTriggerPtr(new SoundTrigger(s, 50.0f)));
}

void World::setupSides()
{
	for(int i = 0; i < NUM_SIDES; i++) {
		addPlatoon(i);
		addDictator(i);
	}
}

SoldierPtr World::addSoldier(bool first, SoldierRank rank, WarriorType wt, bool dictator)
{
	SoldierPtr s = SoldierPtr(new Soldier(shared_from_this(), first, rank, wt));
	s->init();
	int id = s->getID();
	mSoldiers.insert(std::make_pair(id, s));

	if(dictator) {
		s->setDictator(true);
		s->clearWeapons();
		s->addWeapon(WeaponPtr(new Pistol()));
	}
	else {
		mSoldiersAlive[first ? 0 : 1]++;
	}

	float x = mWidth * 0.3f;
	float y = mHeight * 0.3f;
	if(first) {
		x = -x;
		y = -y;
	}
	s->setPosition(Vector3(x, y, 0.0f));
	return s;
}

void World::addTrees()
{
	int squareSide = 64;
	int numXSquares = mWidth / squareSide;
	int numYSquares = mHeight / squareSide;

	for(int k = -numYSquares / 2; k < numYSquares / 2; k++) {
		for(int j = -numXSquares / 2; j < numXSquares / 2; j++) {
			if((k == -numYSquares / 2 && j == -numXSquares / 2) ||
				       (k == numYSquares / 2 - 1 && j == numXSquares / 2 -1))
				continue;

			for(int i = 0; i < 10; i++) {
				float x = Random::uniform();
				float y = Random::uniform();
				float r = Random::uniform();

				x *= squareSide;
				y *= squareSide;
				x += j * squareSide;
				y += k * squareSide;
				r = Common::clamp(3.0f, r * 8.0f, 8.0f);

				bool tooclose = false;
				for(auto t : mTrees) {
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
				mTrees.push_back(tree);
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
	for(auto t : getTreesAt(s->getPosition(), s->getRadius())) {
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

	assert(s->getSideNum() < NUM_SIDES);
	assert(mSoldiersAlive[s->getSideNum()] > 0);
	mSoldiersAlive[s->getSideNum()]--;
}

void World::updateTriggerSystem(float time)
{
	std::vector<SoldierPtr> soldiers;
	for(auto s : mSoldiers) {
		soldiers.push_back(s.second);
	}
	mTriggerSystem.update(soldiers, time);
}

void World::addPlatoon(int side)
{
	for(int k = 0; k < 4; k++) {
		addSquad(side);
	}
}

void World::addSquad(int side)
{
	SoldierPtr squadleader;
	for(int j = 0; j < 9; j++) {
		WarriorType wt = WarriorType::Soldier;
		if(j == 8)
			wt = WarriorType::Vehicle;

		auto s = addSoldier(side == 0, j == 0 ? SoldierRank::Sergeant : SoldierRank::Private, wt, false);
		if(j == 0) {
			squadleader = s;
		}
		else {
			if(j == 2) {
				s->clearWeapons();
				s->addWeapon(WeaponPtr(new MachineGun()));
			}
			else if(j == 3 || j == 4 || j == 5) {
				s->addWeapon(WeaponPtr(new Bazooka()));
			}
			squadleader->addCommandee(s);
		}
	}
}

void World::addDictator(int side)
{
	addSoldier(side == 0, SoldierRank::Private, WarriorType::Soldier, true);
}

}


