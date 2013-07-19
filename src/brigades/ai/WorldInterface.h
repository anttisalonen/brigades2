#ifndef BRIGADES_AI_WORLDINTERFACE_H
#define BRIGADES_AI_WORLDINTERFACE_H

#include <deque>
#include <set>

#include "common/Clock.h"
#include "common/Rectangle.h"

#include "brigades/World.h"

namespace Brigades {

namespace AI {

class WorldInterface {
	public:
		WorldInterface(WorldPtr p);
	private:
		WorldPtr mWorld;
};

}

}

#endif

