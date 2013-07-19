#ifndef BRIGADES_SOLDIERCONTROLLER_H
#define BRIGADES_SOLDIERCONTROLLER_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "Soldier.h"

#include "common/Vehicle.h"
#include "common/Clock.h"
#include "common/Steering.h"


namespace Brigades {

class Soldier;
class World;
class SoldierQuery;

class SoldierController : public boost::enable_shared_from_this<SoldierController> {
	public:
		SoldierController(boost::shared_ptr<Soldier> s);
		virtual ~SoldierController() { }
		SoldierQuery getControlledSoldier() const;
		void update(float time);
		bool handleAttackOrder(const AttackOrder& r);
		bool handleAttackSuccess(boost::shared_ptr<Soldier> s, const AttackOrder& r);
		void handleAttackFailure(boost::shared_ptr<Soldier> s, const AttackOrder& r);
		void handleReinforcement(boost::shared_ptr<Soldier> s);
		Common::Vector3 createMovement(bool defmov, const Common::Vector3& mov) const;

	private:
		Common::Vector3 defaultMovement() const;
		bool moveTo(const Common::Vector3& dir, float time, bool autorotate);
		bool turnTo(const Common::Vector3& dir);
		bool turnBy(float rad);
		bool setVelocityToHeading();
		bool setVelocityToNegativeHeading();
		boost::shared_ptr<Common::Steering> getSteering();
		bool handleLeaderCheck(float time);

		bool checkLeaderStatus();
		void updateObstacleCache();

		boost::shared_ptr<World> mWorld;
		boost::shared_ptr<Soldier> mSoldier;
		boost::shared_ptr<Common::Steering> mSteering;
		Common::SteadyTimer mLeaderStatusTimer;

		std::vector<Common::Obstacle*> mObstacleCache;
		Common::SteadyTimer mObstacleCacheTimer;
		Common::Countdown mMovementSoundTimer;

		friend class SoldierAction;
};

typedef boost::shared_ptr<SoldierController> SoldierControllerPtr;

}

#endif

