#include <stdio.h>

#include "AgentDirectory.h"
#include "ai/SoldierAgent.h"

namespace Brigades {

bool AgentDirectory::addAgent(const SoldierPtr s, boost::shared_ptr<SoldierAgent> a)
{
	printf("Adding agent %p for soldier %p - have %zd agents\n", a.get(), s.get(), mAgents.size());
	auto sold = mAgents.find(s);
	if(sold == mAgents.end()) {
		mAgents.insert({s, a});
		return true;
	} else {
		return false;
	}
}

bool AgentDirectory::freeSoldier(const SoldierPtr s)
{
	auto pair = mAgents.find(s);
	if(pair == mAgents.end())
		return false;

	mAgents.erase(pair);
	return true;
}

bool AgentDirectory::removeAgent(const SoldierPtr s, boost::shared_ptr<SoldierAgent> a)
{
	printf("Removing agent %p for soldier %p\n", a.get(), s.get());
	auto pair = mAgents.find(s);
	if(pair == mAgents.end())
		return false;

	if(pair->second == a) {
		mAgents.erase(pair);
		return true;
	}

	return false;
}

std::map<SoldierPtr, boost::shared_ptr<SoldierAgent>>& AgentDirectory::getAgents()
{
	return mAgents;
}

void AgentDirectory::soldierAdded(SoldierPtr p)
{
	auto controller = SoldierControllerPtr(new SoldierController(p));
	auto a = boost::shared_ptr<SoldierAgent>(new AI::SoldierAgent(controller));
	auto pair = mAgents.find(p);
	assert(pair == mAgents.end());
	mAgents.insert({p, a});
}

void AgentDirectory::soldierRemoved(SoldierPtr p)
{
	auto pair = mAgents.find(p);
	assert(pair != mAgents.end());
	mAgents.erase(pair);
}


}

