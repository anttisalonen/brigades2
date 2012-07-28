#include <iostream>

#include <stdlib.h>

#include "World.h"
#include "Driver.h"

using namespace Brigades;

int main(int argc, char** argv)
{
	std::cout << "Brigades\n";

	WorldPtr world(new World());
	DriverPtr driver(new Driver(world));

	srand(time(NULL));
	world->create();
	driver->init();


	driver->run();

	return 0;
}

