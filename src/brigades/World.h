#ifndef BRIGADES_WORLD_H
#define BRIGADES_WORLD_H

#include <vector>
#include <map>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "common/Entity.h"
#include "common/Steering.h"

#define NUM_SIDES 2

namespace Brigades {

class World;

class Tree : public Common::Entity {
	public:
		Tree(boost::shared_ptr<World> w);
};

typedef boost::shared_ptr<Tree> TreePtr;

class Side {
	public:
		Side(bool first);
		bool isFirst() const;

	private:
		bool mFirst;
};

typedef boost::shared_ptr<Side> SidePtr;

class Soldier : public Common::Entity {
	public:
		Soldier(boost::shared_ptr<World> w, bool firstside);
		SidePtr getSide() const;
		int getID() const;
		void update(float time) override;

	private:
		SidePtr mSide;
		int mID;
		Common::Steering mSteering;
		static int getNextID();
};

typedef boost::shared_ptr<Soldier> SoldierPtr;

class SoldierAction {
};

typedef boost::shared_ptr<SoldierAction> SoldierActionPtr;

class World : public boost::enable_shared_from_this<World> {

	public:
		World();
		void create();

		// accessors
		std::vector<TreePtr> getTreesAt(const Common::Vector3& v, float radius) const;
		std::vector<SoldierPtr> getSoldiersAt(const Common::Vector3& v, float radius) const;
		float getWidth() const;
		float getHeight() const;
		SidePtr getSide(bool first) const;

		// modifiers
		void update(float time);
		bool addSoldierAction(const SoldierPtr s, const SoldierAction& a);

	private:
		void setupSides();
		void addSoldier(bool first);
		float mWidth;
		float mHeight;
		SidePtr mSides[NUM_SIDES];
		std::map<int, SoldierPtr> mSoldiers;
};

typedef boost::shared_ptr<World> WorldPtr;

};

#endif
