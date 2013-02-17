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
		SoldierController();
		SoldierController(boost::shared_ptr<Soldier> s);
		virtual ~SoldierController() { }
		void setSoldier(boost::shared_ptr<Soldier> s);
		virtual void act(float time) = 0;
		virtual bool handleAttackOrder(const AttackOrder& r) = 0;
		virtual bool handleAttackSuccess(boost::shared_ptr<Soldier> s, const AttackOrder& r) = 0;
		virtual void handleAttackFailure(boost::shared_ptr<Soldier> s, const AttackOrder& r) = 0;
		virtual void handleReinforcement(boost::shared_ptr<Soldier> s) = 0;

		Common::Vector3 defaultMovement(float time);
		void moveTo(const Common::Vector3& dir, float time, bool autorotate);
		void turnTo(const Common::Vector3& dir);
		void turnBy(float rad);
		void setVelocityToHeading();
		boost::shared_ptr<Common::Steering> getSteering();
		bool handleLeaderCheck(float time);

		virtual void say(boost::shared_ptr<Soldier> s, const char* msg) { }

	protected:
		boost::shared_ptr<World> mWorld;
		boost::shared_ptr<Soldier> mSoldier;
		boost::shared_ptr<Common::Steering> mSteering;
		Common::SteadyTimer mLeaderStatusTimer;

	private:
		bool checkLeaderStatus();
		void updateObstacleCache();

		std::vector<Common::Obstacle*> mObstacleCache;
		Common::SteadyTimer mObstacleCacheTimer;
};

typedef boost::shared_ptr<SoldierController> SoldierControllerPtr;

}

#endif

