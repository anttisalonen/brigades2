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
	SoldierRank r = SoldierRank::Private;

	bool observer = false;
	bool debug = false;

	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-o")) {
			observer = true;
		}
		if(!strcmp(argv[i], "-d")) {
			debug = true;
		}
		if(!strcmp(argv[i], "-r")) {
			bool err = false;
			if(++i >= argc) {
				err = true;
			} else {
				if(!strcmp(argv[i], "private")) {
					r = SoldierRank::Private;
				} else if(!strcmp(argv[i], "sergeant")) {
					r = SoldierRank::Sergeant;
				} else if(!strcmp(argv[i], "lieutenant")) {
					r = SoldierRank::Lieutenant;
				} else if(!strcmp(argv[i], "captain")) {
					r = SoldierRank::Captain;
				} else {
					err = true;
				}
			}

			if(err) {
				std::cout << "-r requires an argument: private, sergeant, lieutenant.\n";
				exit(1);
			}
		}
	}
	WorldPtr world(new World());
	DriverPtr driver(new Driver(world, observer, r));
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

