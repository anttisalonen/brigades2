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

Weapon::Weapon(float range, float velocity, float loadtime)
	: mRange(range),
	mVelocity(velocity),
	mLoadTime(loadtime)
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

void Weapon::shoot(WorldPtr w, const SoldierPtr s, const Vector3& dir)
{
	if(!canShoot())
		return;

	w->addBullet(shared_from_this(), s, dir);
	mLoadTime.rewind();
}

/* NOTE: increasing bullet speed is an excellent way to
 * make the game harder. */
AssaultRifle::AssaultRifle()
	: Weapon(25.0f, 20.0f, 0.1f)
{
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

	Vector3 obs = mSteering->obstacleAvoidance(obstacles);
	Vector3 wal = mSteering->wallAvoidance(walls);

	Vector3 tot;
	mSteering->accumulate(tot, wal);
	mSteering->accumulate(tot, obs);

	return tot;
}

void SoldierController::moveTo(const Common::Vector3& dir, float time, bool autorotate)
{
	if(dir.null() && !mSoldier->getVelocity().null()) {
		mSoldier->setAcceleration(mSoldier->getVelocity() * -1.0f);
		return;
	}
	mSoldier->setAcceleration(dir * (10.0f / time));
	mSoldier->Vehicle::update(time);
	if(autorotate && mSoldier->getVelocity().length() > 0.3f)
		mSoldier->setAutomaticHeading();
}

void SoldierController::turnTo(const Common::Vector3& dir)
{
	mSoldier->setXYRotation(atan2(dir.y, dir.x));
}

bool SoldierController::handleEvents()
{
	bool ret = !mSoldier->getEvents().empty();

	for(auto e : mSoldier->getEvents()) {
		switch(e->getType()) {
			case EventType::Sound:
				mSoldier->getSensorySystem()->addSound(static_cast<Soldier*>(e->getData())->shared_from_this());
				break;
		}
	}
	mSoldier->getEvents().clear();

	return ret;
}

Soldier::Soldier(boost::shared_ptr<World> w, bool firstside)
	: Common::Vehicle(0.5f, 10.0f, 100.0f),
	mWorld(w),
	mSide(w->getSide(firstside)),
	mID(getNextID()),
	mFOV(PI),
	mAlive(true),
	mWeapon(WeaponPtr(new AssaultRifle()))
{
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

	mWeapon->update(time);
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

void Soldier::die()
{
	mAlive = false;
}

bool Soldier::isDead() const
{
	return !mAlive;
}

WeaponPtr Soldier::getWeapon()
{
	return mWeapon;
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

Bullet::Bullet(const SoldierPtr shooter, const Vector3& pos, const Vector3& vel, float timeleft)
	: mShooter(shooter),
	mTimer(timeleft)
{
	mTimer.rewind();
	mPosition = pos;
	mVelocity = vel;
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

World::World()
	: mWidth(100.0f),
	mHeight(100.0f),
	mVisibility(25.0f),
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
			if(s == (*bit)->getShooter())
				continue;

			if(s->isDead())
				continue;

			if(Math::segmentCircleIntersect((*bit)->getPosition(),
						(*bit)->getPosition() + (*bit)->getVelocity() * time,
						s->getPosition(), s->getRadius())) {
				killSoldier(s);
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
		for(int j = 0; j < 10; j++)
			addSoldier(i == 0);
	}
}

void World::addSoldier(bool first)
{
	SoldierPtr s = SoldierPtr(new Soldier(shared_from_this(), first));
	s->init();
	int id = s->getID();
	mSoldiers.insert(std::make_pair(id, s));
	mSoldiersAlive[first ? 0 : 1]++;

	float x = mWidth * 0.3f;
	float y = mHeight * 0.3f;
	if(first) {
		x = -x;
		y = -y;
	}
	s->setPosition(Vector3(x, y, 0.0f));
}

void World::addTrees()
{
	for(int i = 0; i < mWidth * mHeight * 0.01f; i++) {
		float x = Random::clamped();
		float y = Random::clamped();
		float r = Random::uniform();

		x *= mWidth;
		y *= mHeight;
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
	if(fabs(s->getPosition().x) > mWidth * 0.5f || fabs(s->getPosition().y) > mHeight * 0.5f) {
		// stepped out of the world and fell
		killSoldier(s);
	}
}

void World::checkForWin()
{
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

}

