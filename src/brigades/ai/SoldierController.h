#ifndef BRIGADES_AI_SOLDIERCONTROLLER_H
#define BRIGADES_AI_SOLDIERCONTROLLER_H

#include <deque>

#include "common/Clock.h"
#include "common/Rectangle.h"

#include "brigades/World.h"

namespace Brigades {

namespace AI {

class Goal {
	public:
		Goal(SoldierPtr s);
		virtual ~Goal() { }
		virtual void activate() { }
		virtual bool process(float time) = 0;
		virtual void deactivate() { }
		virtual void addSubGoal(boost::shared_ptr<Goal> g) = 0;
		virtual bool handleAttackOrder(const Common::Rectangle& r);
		virtual bool handleAttackSuccess(const Common::Rectangle& r);

	protected:
		SoldierPtr mSoldier;
		WorldPtr mWorld;
};

typedef boost::shared_ptr<Goal> GoalPtr;

class AtomicGoal : public Goal {
	public:
		AtomicGoal(SoldierPtr s);
		void addSubGoal(GoalPtr g) override;
};

class CompositeGoal : public Goal {
	public:
		CompositeGoal(SoldierPtr s);
		void addSubGoal(GoalPtr g) override;
		bool processSubGoals(float time);
		void emptySubGoals();
		void activateIfInactive();
		virtual void deactivate();

	protected:
		std::deque<GoalPtr> mSubGoals;

	private:
		bool mActive;
};

class PrivateGoal : public CompositeGoal {
	public:
		PrivateGoal(SoldierPtr s);
		bool process(float time);
		bool handleAttackOrder(const Common::Rectangle& r);

	private:
};

class SquadLeaderGoal : public CompositeGoal {
	public:
		SquadLeaderGoal(SoldierPtr s);
		bool process(float time);
		bool handleAttackOrder(const Common::Rectangle& r);

	private:
		Common::Rectangle mArea;
};

class PlatoonLeaderGoal : public CompositeGoal {
	public:
		PlatoonLeaderGoal(SoldierPtr s);
		void activate();
		bool process(float time);
		bool handleAttackSuccess(const Common::Rectangle& r);

	private:
		void markHome(const Common::Vector3& v);
		void reduceHome(const Common::Vector3& v);
		void markEnemy(const Common::Vector3& v);
		void markCombat(const Common::Vector3& v);
		void giveOrders();
		bool calculateAttackPosition(const SoldierPtr s, Common::Vector3& pos);
		std::pair<unsigned int, unsigned int> coordinateToSector(const Common::Vector3& v) const;
		unsigned int sectorToIndex(unsigned int i, unsigned int j) const;
		unsigned int sectorToIndex(const std::pair<unsigned int, unsigned int>& s) const;
		unsigned int coordinateToIndex(const Common::Vector3& v) const;
		Common::Vector3 sectorToCoordinate(const std::pair<unsigned int, unsigned int>& s) const;
		bool criticalSquad(const SoldierPtr p) const;
		Common::Rectangle getSector(const std::pair<unsigned int, unsigned int>& s) const;
		Common::Rectangle getSector(const Common::Vector3& v) const;

		std::vector<signed char> mSectorMap;
		unsigned int mNumXSectors;
		unsigned int mNumYSectors;

};

class SeekAndDestroyGoal : public AtomicGoal {
	public:
		SeekAndDestroyGoal(SoldierPtr s, const Common::Rectangle& r);
		bool process(float time);

	private:
		void updateCommandeeOrders();
		void updateShootTarget();
		void move(float time);
		void updateTargetSoldier();
		void tryToShoot();

		Common::SteadyTimer mTargetUpdateTimer;
		SoldierPtr mTargetSoldier;
		Common::Vector3 mShootTargetPosition;
		Common::SteadyTimer mCommandTimer;
		bool mRetreat;
		Common::Rectangle mArea;
};

class SoldierController : public Brigades::SoldierController {
	public:
		SoldierController(SoldierPtr p);
		void act(float time);
		bool handleAttackOrder(const Common::Rectangle& r);
		bool handleAttackSuccess(const Common::Rectangle& r);

	private:
		bool checkLeaderStatus();
		void resetRootGoal();

		GoalPtr mCurrentGoal;
};

}

}

#endif

