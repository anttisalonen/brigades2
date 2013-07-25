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
		if(!soldier.mounted() && (!pcv.null() || mInputState->isShooting())) {
			actions.push_back(SoldierAction(SAType::StopDigging));
		} else {
			return actions;
		}
	}

	Vector3 mousedir = mInputState->getMousePositionOnField() - soldier.getPosition();

	if(soldier.driving()) {
		Vector3 tot = createMovement(soldier.getMountPoint().getHeadingVector() * pcv.y);
		actions.push_back(SoldierAction(SAType::Move, tot));
		if(pcv.x) {
			float rot(pcv.x);
			rot *= -0.05f;
			actions.push_back(SoldierAction(SAType::TurnBy, rot));
			if(soldier.getMountPoint().getHeadingVector().dot(soldier.getMountPoint().getVelocity()) > 0.0f) {
				actions.push_back(SoldierAction(SAType::SetVelocityToHeading));
			} else {
				actions.push_back(SoldierAction(SAType::SetVelocityToNegativeHeading));
			}
		}
	} else if(!soldier.mounted()) {
		actions.push_back(SoldierAction(SAType::Turn, mousedir));
		Vector3 tot = createMovement(pcv);
		actions.push_back(SoldierAction(SAType::Move, tot));
	}

	if(mInputState->isShooting()) {
		if(soldier.hasCurrentWeapon() && soldier.getCurrentWeapon().canShoot()) {
			actions.push_back(SoldierAction(SAType::Shoot, mousedir));
		}
	}

	return actions;
}

void PlayerAgent::newCommunication(const SoldierCommunication& comm)
{
	switch(comm.comm) {
		case CommunicationType::Order:
			printf("Received order\n");
			break;
		case CommunicationType::Acknowledgement:
			printf("Received ack\n");
			break;
		case CommunicationType::ReportSuccess:
			printf("Received success\n");
			break;
		case CommunicationType::ReportFail:
			printf("Received fail\n");
			break;
	}
}

}

