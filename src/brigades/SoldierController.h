#ifndef BRIGADES_SOLDIERCONTROLLER_H
#define BRIGADES_SOLDIERCONTROLLER_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "common/Vehicle.h"
#include "common/Clock.h"
#include "common/Steering.h"


namespace Brigades {

class Soldier;
class World;
class SoldierQuery;

struct AttackOrder {
	AttackOrder() { }
	AttackOrder(const Common::Vector3& p)
		: CenterPoint(p) { }
	AttackOrder(const Common::Vector3& p, const Common::Vector3& d)
		: CenterPoint(p),
		DefenseLineToRight(d) { }
	AttackOrder(const Common::Vector3& p, const Common::Vector3& d, float width);
	Common::Vector3 CenterPoint;
	Common::Vector3 DefenseLineToRight;
};

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
		void moveTo(const Common::Vector3& dir, float time, bool autorotate);
		void turnTo(const Common::Vector3& dir);
		void turnBy(float rad);
		void setVelocityToHeading();
		boost::shared_ptr<Common::Steering> getSteering();
		bool handleLeaderCheck(float time);

		virtual void say(boost::shared_ptr<Soldier> s, const char* msg) { }

		bool checkLeaderStatus();
		void updateObstacleCache();

		boost::shared_ptr<World> mWorld;
		boost::shared_ptr<Soldier> mSoldier;
		boost::shared_ptr<Common::Steering> mSteering;
		Common::SteadyTimer mLeaderStatusTimer;

		std::vector<Common::Obstacle*> mObstacleCache;
		Common::SteadyTimer mObstacleCacheTimer;
		Common::Countdown mMovementSoundTimer;
};

typedef boost::shared_ptr<SoldierController> SoldierControllerPtr;

}

#endif

