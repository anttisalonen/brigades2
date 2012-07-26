#ifndef BRIGADES_WORLD_H
#define BRIGADES_WORLD_H

#include <vector>

#include <boost/shared_ptr.hpp>

#include "common/Entity.h"

namespace Brigades {

class World;

class Tree : public Common::Entity<boost::shared_ptr<World>> {
	public:
		Tree(boost::shared_ptr<World> w);
};

typedef boost::shared_ptr<Tree> TreePtr;

class Soldier : public Common::Entity<boost::shared_ptr<World>> {
	public:
		Soldier(boost::shared_ptr<World> w);
};

typedef boost::shared_ptr<Soldier> SoldierPtr;

class SoldierAction {
};

typedef boost::shared_ptr<SoldierAction> SoldierActionPtr;

class World {

	public:
		World();

		// accessors
		std::vector<TreePtr> getTreesAt(const Vector3& v, float radius) const;
		std::vector<SoldierPtr> getSoldiersAt(const Vector3& v, float radius) const;
		float getWidth() const;
		float getHeight() const;

		// modifiers
		void update(float time);
		bool addSoldierAction(const SoldierPtr s, const SoldierAction& a);

	protected:
		float mWidth;
		float mHeight;
};

typedef boost::shared_ptr<World> WorldPtr;

};

#endif
