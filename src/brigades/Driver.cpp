#include "Driver.h"

namespace Brigades {

Driver::Driver(WorldPtr w)
	: mWorld(w)
{
}

void Driver::run()
{
	std::cout << "Hello world!\n";
}


}
