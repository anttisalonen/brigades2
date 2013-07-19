#ifndef BRIGADES_AGENTDIRECTORY_H
#define BRIGADES_AGENTDIRECTORY_H

#include <boost/shared_ptr.hpp>

#include "World.h"
#include "Soldier.h"
#include "SoldierController.h"

namespace Brigades {

class SoldierAgent;

class AgentDirectory : public SoldierListener {
	public:
		bool addAgent(const SoldierPtr s, boost::shared_ptr<SoldierController> c, boost::shared_ptr<SoldierAgent> a);
		bool freeSoldier(const SoldierPtr s);
		bool removeAgent(const SoldierPtr s, boost::shared_ptr<SoldierAgent> a);
		std::map<SoldierPtr, std::pair<boost::shared_ptr<SoldierController>, boost::shared_ptr<SoldierAgent>>>& getAgents();
		virtual void soldierAdded(SoldierPtr p) override;
		virtual void soldierRemoved(SoldierPtr p) override;

		boost::shared_ptr<SoldierController> getControllerFor(const boost::shared_ptr<Soldier> s);

	private:
		std::map<SoldierPtr, std::pair<boost::shared_ptr<SoldierController>, boost::shared_ptr<SoldierAgent>>> mAgents;
};

}

#endif

