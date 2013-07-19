#ifndef BRIGADES_PLAYERAGENT_H
#define BRIGADES_PLAYERAGENT_H

#include <boost/shared_ptr.hpp>

#include "SoldierAgent.h"
#include "SoldierAction.h"

namespace Brigades {

class SoldierController;
class SoldierAction;
class InputState;

class PlayerAgent : public SoldierAgent {
	public:
		PlayerAgent(boost::shared_ptr<SoldierController> s, const InputState* is);
		virtual std::vector<SoldierAction> update(float time) override;
		virtual void newCommunication(const SoldierCommunication& comm) override;

	private:
		const InputState* mInputState;
};

}


#endif

