#include "World.h"

#include "common/Random.h"

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

Soldier::Soldier(boost::shared_ptr<World> w, bool firstside)
	: Common::Vehicle(0.5f, 10.0f, 100.0f),
	mWorld(w),
	mSide(w->getSide(firstside)),
	mID(getNextID()),
	mSteering(*this)
{
}

SidePtr Soldier::getSide() const
{
	return mSide;
}

int Soldier::getID() const
{
	return mID;
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

	std::vector<boost::shared_ptr<Tree>> trees = mWorld->getTreesAt(mPosition, mVelocity.length());
	std::vector<Obstacle*> obstacles(trees.size());
	for(unsigned int i = 0; i < trees.size(); i++)
		obstacles[i] = trees[i].get();

	std::vector<WallPtr> wallptrs = mWorld->getWallsAt(mPosition, mVelocity.length());
	std::vector<Wall*> walls(wallptrs.size());
	for(unsigned int i = 0; i < wallptrs.size(); i++)
		walls[i] = wallptrs[i].get();

	Vector3 obs = mSteering.obstacleAvoidance(obstacles);
	Vector3 wal = mSteering.wallAvoidance(walls);
	Vector3 vel = mSteering.wander();
	Vector3 tot;
	mSteering.accumulate(tot, wal);
	mSteering.accumulate(tot, obs);
	mSteering.accumulate(tot, vel);
	mAcceleration = tot * (10.0f / time);
	Vehicle::update(time);
	setAutomaticHeading();
}

World::World()
	: mWidth(100.0f),
	mHeight(100.0f)
{
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


// modifiers
void World::update(float time)
{
	for(auto s : mSoldiers) {
		s.second->update(time);
	}
}

bool World::addSoldierAction(const SoldierPtr s, const SoldierAction& a)
{
	/* TODO */
	return true;
}

void World::setupSides()
{
	for(int i = 0; i < NUM_SIDES; i++) {
		addSoldier(i == 0);
	}
}

void World::addSoldier(bool first)
{
	SoldierPtr s = SoldierPtr(new Soldier(shared_from_this(), first));
	int id = s->getID();
	mSoldiers.insert(std::make_pair(id, s));

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
	Vector3 a(-mWidth * 0.5f + 5.0f, -mHeight * 0.5f + 5.0f, 0.0f);
	Vector3 b( mWidth * 0.5f - 5.0f, -mHeight * 0.5f + 5.0f, 0.0f);
	Vector3 c(-mWidth * 0.5f + 5.0f,  mHeight * 0.5f - 5.0f, 0.0f);
	Vector3 d( mWidth * 0.5f - 5.0f,  mHeight * 0.5f - 5.0f, 0.0f);

	mWalls.push_back(WallPtr(new Wall(a, b)));
	mWalls.push_back(WallPtr(new Wall(a, c)));
	mWalls.push_back(WallPtr(new Wall(b, d)));
	mWalls.push_back(WallPtr(new Wall(c, d)));
}

}

