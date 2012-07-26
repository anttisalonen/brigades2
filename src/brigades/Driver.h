#ifndef BRIGADES_DRIVER_H
#define BRIGADES_DRIVER_H

#include <boost/shared_ptr.hpp>

#include "World.h"

namespace Brigades {

class Driver {

	public:
		Driver(WorldPtr w);
		void run();

	protected:
		WorldPtr mWorld;

};

typedef boost::shared_ptr<Driver> DriverPtr;

};

#endif

