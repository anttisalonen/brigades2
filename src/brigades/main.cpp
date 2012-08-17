#include <iostream>

#include <stdlib.h>
#include <string.h>

#include "World.h"
#include "Driver.h"
#include "DebugOutput.h"

using namespace Brigades;

int main(int argc, char** argv)
{
	std::cout << "Brigades\n";

	bool observer = false;
	bool debug = false;

	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-o")) {
			observer = true;
		}
		if(!strcmp(argv[i], "-d")) {
			debug = true;
		}
	}
	WorldPtr world(new World());
	DriverPtr driver(new Driver(world, observer));
	if(debug)
		DebugOutput::setInstance(driver);

	int seed = time(NULL);
	srand(seed);
	std::cout << "Seed: " << seed << "\n";
	world->create();
	driver->init();


	driver->run();

	return 0;
}

