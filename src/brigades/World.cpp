#include "World.h"

using namespace Common;

namespace Brigades {

Tree::Tree(boost::shared_ptr<World> w)
	: Entity<WorldPtr>(w)
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

Soldier::Soldier(boost::shared_ptr<World> w, bool firstside)
	: Entity<WorldPtr>(w),
	mSide(w->getSide(firstside)),
	mID(getNextID())
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
	mAcceleration.x = 10.0f;
	updateComplete(time, 10.0f);
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
	setupSides();
}

// accessors
std::vector<TreePtr> World::getTreesAt(const Vector3& v, float radius) const
{
	/* TODO */
	return std::vector<TreePtr>();
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

	float x = mWidth * 0.4f;
	float y = mHeight * 0.4f;
	if(first) {
		x = -x;
		y = -y;
	}
	s->setPosition(Vector3(x, y, 0.0f));
}



}
