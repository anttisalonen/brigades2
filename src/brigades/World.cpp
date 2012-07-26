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
	mSide(w->getSide(firstside))
{
}

SidePtr Soldier::getSide() const
{
	return mSide;
}

World::World()
	: mWidth(100.0f),
	mHeight(100.0f)
{
	for(int i = 0; i < NUM_SIDES; i++) {
		mSides[i] = SidePtr(new Side(i == 0));
	}
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
	return std::vector<SoldierPtr>();
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
	/* TODO */
}

bool World::addSoldierAction(const SoldierPtr s, const SoldierAction& a)
{
	/* TODO */
	return true;
}



}
