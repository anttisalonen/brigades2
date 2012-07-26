#ifndef BRIGADES_WORLD_H
#define BRIGADES_WORLD_H

#include <boost/shared_ptr.hpp>

namespace Brigades {

class World {

	public:
		World();

};

typedef boost::shared_ptr<World> WorldPtr;

};

#endif
