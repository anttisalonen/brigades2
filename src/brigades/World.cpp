#include "World.h"

using namespace Common;

namespace Brigades {

Tree::Tree(boost::shared_ptr<World> w)
	: Entity<WorldPtr>(w)
{
}

Soldier::Soldier(boost::shared_ptr<World> w)
	: Entity<WorldPtr>(w)
{
}

World::World()
	: mWidth(100.0f),
	mHeight(100.0f)
{
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
