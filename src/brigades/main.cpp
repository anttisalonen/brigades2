#include <iostream>

#include <stdlib.h>
#include <string.h>

#include "Armory.h"
#include "World.h"
#include "Driver.h"
#include "DebugOutput.h"

using namespace Brigades;
using namespace Common;

template<typename T>
bool getParameter(int argc, char** argv, int& i,
		const char* arg, const std::map<const char*, T>& options, T& value)
{
	if(strcmp(argv[i], arg))
		return true;

	if(++i < argc) {
		for(auto o : options) {
			if(!strcmp(argv[i], o.first)) {
				value = o.second;
				return true;
			}
		}
	}

	std::cerr << arg << " requires an argument - one of: ";
	for(auto o : options) {
		std::cerr << o.first << " ";
	}
	std::cerr << "\n";
	return false;
}

class ArcadeArmory : public Armory {
	public:
		ArcadeArmory();
};

class SimulationArmory : public Armory {
	public:
		SimulationArmory();
};

SimulationArmory::SimulationArmory()
{
	mAssaultRifle = boost::shared_ptr<WeaponType>(new WeaponType("Assault Rifle",
				170.0f, 200.0f, 0.1f,
				Math::degreesToRadians(1.0f), true));
	mMachineGun = boost::shared_ptr<WeaponType>(new WeaponType("Machine Gun",
				180.0f, 200.0f, 0.04f,
				Math::degreesToRadians(2.0f), true));
	mBazooka = boost::shared_ptr<WeaponType>(new WeaponType("Bazooka",
				150.0f, 160.0f, 4.0f,
				Math::degreesToRadians(0.5f), true,
				1.0f, 1.0f, 1.0f));
	mPistol = boost::shared_ptr<WeaponType>(new WeaponType("Pistol",
				100.0f, 180.0f, 0.5f,
				Math::degreesToRadians(2.0f), true));
	mAutoCannon = boost::shared_ptr<WeaponType>(new WeaponType("Automatic Cannon",
				200.0f, 200.0f, 0.25f,
				Math::degreesToRadians(1.0f), false, 1.0f, 1.0f, 0.0f));
}

ArcadeArmory::ArcadeArmory()
{
	mAssaultRifle = boost::shared_ptr<WeaponType>(new WeaponType("Assault Rifle",
				25.0f, 20.0f, 0.1f,
				Math::degreesToRadians(5.0f), false));
	mMachineGun = boost::shared_ptr<WeaponType>(new WeaponType("Machine Gun",
				35.0f, 20.0f, 0.04f,
				Math::degreesToRadians(5.0f), false));
	mBazooka = boost::shared_ptr<WeaponType>(new WeaponType("Bazooka",
				30.0f, 16.0f, 4.0f,
				Math::degreesToRadians(5.0f), false,
				1.0f, 1.0f, 1.0f));
	mPistol = boost::shared_ptr<WeaponType>(new WeaponType("Pistol",
				15.0f, 18.0f, 0.5f,
				Math::degreesToRadians(5.0f), false));
	mAutoCannon = boost::shared_ptr<WeaponType>(new WeaponType("Automatic Cannon",
				40.0f, 20.0f, 0.25f,
				Math::degreesToRadians(5.0f), false, 1.0f, 1.0f, 0.0f));
}

int main(int argc, char** argv)
{
	std::cout << "Brigades\n";
	SoldierRank r = SoldierRank::Private;

	bool observer = false;
	bool debug = false;
	bool arcade = false;
	bool skirmish = false;

	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-o")) {
			observer = true;
		}
		else if(!strcmp(argv[i], "-d")) {
			debug = true;
		}
		else if(!strcmp(argv[i], "--skirmish")) {
			skirmish = true;
		}
		else if(!strcmp(argv[i], "--arcade")) {
			arcade = true;
		}
		else if(!strcmp(argv[i], "-r")) {
			if(!getParameter(argc, argv, i, "-r", { { "private", SoldierRank::Private},
						{"sergeant", SoldierRank::Sergeant},
						{"lieutenant", SoldierRank::Lieutenant},
						{"captain", SoldierRank::Captain} },
						r)) {
				exit(1);
			}
		} else {
			std::cerr << "Unknown parameter '" << argv[i] << "'.\n";
			exit(1);
		}
	}
	float width, height, visibility, sounddistance;
	Armory* a;
	UnitSize u = UnitSize::Company;
	if(arcade) {
		a = new ArcadeArmory();
		visibility = 30.0f;
		sounddistance = 50.0f;
		if(skirmish) {
			width = 256.0f;
			height = 256.0f;
			u = UnitSize::Squad;
		} else {
			width = 512.0f;
			height = 512.0f;
		}
	} else {
		a = new SimulationArmory();
		visibility = 200.0f;
		sounddistance = 300.0f;
		if(skirmish) {
			width = 512.0f;
			height = 512.0f;
			u = UnitSize::Squad;
		} else {
			width = 1536.0f;
			height = 1536.0f;
		}
	}
	WorldPtr world(new World(width, height, visibility, sounddistance, u, u == UnitSize::Company, *a));
	DriverPtr driver(new Driver(world, observer, r));
	if(debug)
		DebugOutput::setInstance(driver);

	InfoChannel::setInstance(driver);

	int seed = time(NULL);
	srand(seed);
	std::cout << "Seed: " << seed << "\n";
	world->create();
	driver->init();


	driver->run();

	delete a;

	return 0;
}

