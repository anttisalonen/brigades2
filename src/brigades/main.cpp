#include <iostream>

#include "World.h"
#include "Driver.h"

using namespace Brigades;

int main(int argc, char** argv)
{
	std::cout << "Brigades\n";

	WorldPtr world(new World());
	world->create();

	DriverPtr driver(new Driver(world));

	driver->run();

	return 0;
}

