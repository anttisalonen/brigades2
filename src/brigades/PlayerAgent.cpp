#include "PlayerAgent.h"
#include "InputState.h"

using namespace Common;

namespace Brigades {

PlayerAgent::PlayerAgent(boost::shared_ptr<SoldierController> s, const InputState* is)
	: SoldierAgent(s),
	mInputState(is)
{
}

std::vector<SoldierAction> PlayerAgent::update(float time)
{
	SoldierQuery soldier = getControlledSoldier();
	std::vector<SoldierAction> actions;
	const Vector3& pcv = mInputState->getPlayerControlVelocity();

	// TODO: event for leader check
#if 0
	if(handleLeaderCheck(time)) {
		DebugOutput::getInstance()->addMessage(soldier, Color::White, "Rank updated");
	}
#endif

	if(mInputState->isDigging()) {
		if(!mInputState->isDriving() && (!pcv.null() || mInputState->isShooting())) {
			actions.push_back(SoldierAction(SAType::StopDigging));
		} else {
			return actions;
		}
	}

	//soldier.handleEvents();
	Vector3 tot;
	Vector3 mousedir = mInputState->getMousePositionOnField() - soldier.getPosition();

	if(!mInputState->isDriving()) {
		tot = createMovement(true, pcv);
		actions.push_back(SoldierAction(SAType::Turn, mousedir));
	} else {
		if(pcv.y) {
			tot = createMovement(false, pcv);
		}
		if(pcv.x) {
			float rot(pcv.x);
			rot *= -0.05f;
			actions.push_back(SoldierAction(SAType::TurnBy, rot));
			if(soldier.getHeadingVector().dot(soldier.getVelocity()) > 0.0f) {
				actions.push_back(SoldierAction(SAType::SetVelocityToHeading));
			} else {
				actions.push_back(SoldierAction(SAType::SetVelocityToNegativeHeading));
			}
		}
	}
	actions.push_back(SoldierAction(SAType::Move, tot));

	if(mInputState->isShooting()) {
		if(soldier.hasCurrentWeapon() && soldier.getCurrentWeapon().canShoot()) {
			actions.push_back(SoldierAction(SAType::Shoot, mousedir));
			//soldier.getCurrentWeapon()->shoot(mWorld, soldier.mousedir);
		}
	}

	return actions;
}

}

