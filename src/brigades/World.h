#ifndef BRIGADES_WORLD_H
#define BRIGADES_WORLD_H

#include <vector>
#include <map>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "common/Vehicle.h"
#include "common/Steering.h"

#define NUM_SIDES 2

namespace Brigades {

class World;

class Tree : public Common::Obstacle {
	public:
		Tree(const Common::Vector3& pos, float radius);
};

typedef boost::shared_ptr<Tree> TreePtr;

class Side {
	public:
		Side(bool first);
		bool isFirst() const;
		int getSideNum() const;

	private:
		bool mFirst;
};

typedef boost::shared_ptr<Side> SidePtr;

class Soldier;

class SoldierController {
	public:
		SoldierController(boost::shared_ptr<World> w, boost::shared_ptr<Soldier> s);
		virtual ~SoldierController() { }
		virtual void act(float time) = 0;

	protected:
		boost::shared_ptr<World> mWorld;
		boost::shared_ptr<Soldier> mSoldier;
		Common::Steering mSteering;
};

typedef boost::shared_ptr<SoldierController> SoldierControllerPtr;

class Soldier : public Common::Vehicle {
	public:
		Soldier(boost::shared_ptr<World> w, bool firstside);
		SidePtr getSide() const;
		int getID() const;
		int getSideNum() const;
		void update(float time) override;
		float getFOV() const; // total FOV in radians
		void setController(SoldierControllerPtr p);

	private:
		boost::shared_ptr<World> mWorld;
		SidePtr mSide;
		int mID;
		float mFOV;
		SoldierControllerPtr mController;

		static int getNextID();
};

typedef boost::shared_ptr<Soldier> SoldierPtr;

class SoldierAction {
	public:
		SoldierAction(SoldierPtr p) : mSoldier(p) { }
		virtual ~SoldierAction() { }
		virtual void apply() = 0;

	protected:
		SoldierPtr mSoldier;
};

typedef boost::shared_ptr<SoldierAction> SoldierActionPtr;

typedef boost::shared_ptr<Common::Wall> WallPtr;

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
		std::vector<WallPtr> getWallsAt(const Common::Vector3& v, float radius) const;
		std::vector<SoldierPtr> getSoldiersInFOV(const SoldierPtr p) const;

		// modifiers
		void update(float time);
		bool addSoldierAction(const SoldierPtr s, const SoldierAction& a);

	private:
		void setupSides();
		void addSoldier(bool first);
		void addTrees();
		void addWalls();
		float mWidth;
		float mHeight;
		SidePtr mSides[NUM_SIDES];
		std::map<int, SoldierPtr> mSoldiers;
		std::vector<TreePtr> mTrees;
		std::vector<WallPtr> mWalls;
		float mVisibility;
};

typedef boost::shared_ptr<World> WorldPtr;

};

#endif
