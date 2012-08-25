#ifndef BRIGADES_AI_SOLDIERCONTROLLER_H
#define BRIGADES_AI_SOLDIERCONTROLLER_H

#include <deque>
#include <set>

#include "common/Clock.h"
#include "common/Rectangle.h"

#include "brigades/World.h"

namespace Brigades {

namespace AI {

class SectorMap {
	public:
		SectorMap(float width, float height, int numx, int numy);
		signed char getValue(const Common::Vector3& v) const;
		signed char getValue(const std::pair<unsigned int, unsigned int>& s) const;
		void setValue(const Common::Vector3& v, signed char x);
		void setAll(signed char c);
		Common::Rectangle getSector(const Common::Vector3& v) const;
		Common::Rectangle getSector(const std::pair<unsigned int, unsigned int>& s) const;
		std::pair<unsigned int, unsigned int> coordinateToSector(const Common::Vector3& v) const;
		Common::Vector3 sectorToCoordinate(const std::pair<unsigned int, unsigned int>& s) const;
		unsigned int getNumXSectors() const;
		unsigned int getNumYSectors() const;

	private:
		unsigned int sectorToIndex(unsigned int i, unsigned int j) const;
		unsigned int sectorToIndex(const std::pair<unsigned int, unsigned int>& s) const;
		unsigned int coordinateToIndex(const Common::Vector3& v) const;

		std::vector<signed char> mSectorMap;
		unsigned int mNumXSectors;
		unsigned int mNumYSectors;
		float mWidth;
		float mHeight;
		float mDimX;
		float mDimY;
};

enum class SubUnitStatus {
	Defending,
	Attacking,
};

class SubUnitHandler {
	public:
		SubUnitHandler(SoldierPtr s);
		bool subUnitsReady() const;
		void updateSubUnitStatus();
		std::map<SoldierPtr, SubUnitStatus>& getStatus();
		const std::map<SoldierPtr, SubUnitStatus>& getStatus() const;

	private:
		SoldierPtr mSoldier;
		std::map<SoldierPtr, SubUnitStatus> mSubUnits;
};

class Goal {
	public:
		Goal(SoldierPtr s);
		virtual ~Goal() { }
		virtual void activate() { }
		virtual bool process(float time) = 0;
		virtual void deactivate() { }
		virtual void addSubGoal(boost::shared_ptr<Goal> g) = 0;
		virtual bool handleAttackOrder(const Common::Rectangle& r);
		virtual bool handleAttackSuccess(SoldierPtr s, const Common::Rectangle& r);

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
		void activate();
		bool process(float time);
		bool handleAttackOrder(const Common::Rectangle& r);

	private:
		void commandDefendPositions();
		Common::Rectangle mArea;
};

class PlatoonLeaderGoal : public CompositeGoal {
	public:
		PlatoonLeaderGoal(SoldierPtr s);
		void activate();
		bool process(float time);
		bool handleAttackSuccess(SoldierPtr s, const Common::Rectangle& r);
		bool handleAttackOrder(const Common::Rectangle& r);

	private:
		std::vector<Common::Rectangle> splitTargetRectangle() const;
		void handleAttackFinish();

		SubUnitHandler mSubUnitHandler;
		Common::Rectangle mTargetRectangle;
		Common::SteadyTimer mSubTimer;
};

class CompanyLeaderGoal : public CompositeGoal {
	public:
		CompanyLeaderGoal(SoldierPtr s);
		void activate();
		bool process(float time);
		bool handleAttackSuccess(SoldierPtr s, const Common::Rectangle& r);

	private:
		std::set<Common::Rectangle> nextAttackSectors() const;
		void updateSectorMap();
		void issueAttackOrders();
		void handleAttackFinish();

		SectorMap mSectorMap;
		SubUnitHandler mSubUnitHandler;
		Common::SteadyTimer mSubTimer;
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
		Common::Countdown mBoredTimer;
		Common::Countdown mEnoughWanderTimer;
};

class SoldierController : public Brigades::SoldierController {
	public:
		SoldierController(SoldierPtr p);
		void act(float time);
		bool handleAttackOrder(const Common::Rectangle& r);
		bool handleAttackSuccess(SoldierPtr s, const Common::Rectangle& r);

	private:
		bool checkLeaderStatus();
		void resetRootGoal();

		GoalPtr mCurrentGoal;
};

}

}

#endif

