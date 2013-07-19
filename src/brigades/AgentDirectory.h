#ifndef BRIGADES_AGENTDIRECTORY_H
#define BRIGADES_AGENTDIRECTORY_H

#include <boost/shared_ptr.hpp>

#include "World.h"
#include "Soldier.h"
#include "SoldierAgent.h"

namespace Brigades {

class AgentDirectory : public SoldierListener {
	public:
		bool addAgent(const SoldierPtr s, boost::shared_ptr<SoldierAgent> a);
		bool freeSoldier(const SoldierPtr s);
		bool removeAgent(const SoldierPtr s, boost::shared_ptr<SoldierAgent> a);
		std::map<SoldierPtr, boost::shared_ptr<SoldierAgent>>& getAgents();
		virtual void soldierAdded(SoldierPtr p) override;
		virtual void soldierRemoved(SoldierPtr p) override;

	private:
		std::map<SoldierPtr, boost::shared_ptr<SoldierAgent>> mAgents;
};

}

#endif

