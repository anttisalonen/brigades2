#ifndef BRIGADES_AI_SOLDIERCONTROLLER_H
#define BRIGADES_AI_SOLDIERCONTROLLER_H

#include <deque>

#include "common/Clock.h"

#include "brigades/World.h"

namespace Brigades {

namespace AI {

class Goal {
	public:
		Goal(SoldierPtr s);
		virtual ~Goal() { }
		virtual void activate() = 0;
		virtual bool process(float time) = 0;
		virtual void deactivate() = 0;
		virtual void addSubGoal(boost::shared_ptr<Goal> g) = 0;

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

	protected:
		std::deque<GoalPtr> mSubGoals;
};

class SeekAndDestroyGoal : public AtomicGoal {
	public:
		SeekAndDestroyGoal(SoldierPtr s);
		void activate();
		bool process(float time);
		void deactivate();

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
};

class SoldierController : public Brigades::SoldierController {
	public:
		SoldierController(SoldierPtr p);
		void act(float time);

	private:
		GoalPtr mCurrentGoal;
};

}

}

#endif

