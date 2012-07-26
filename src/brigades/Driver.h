#ifndef BRIGADES_DRIVER_H
#define BRIGADES_DRIVER_H

#include <boost/shared_ptr.hpp>

#include <SDL.h>
#include <SDL_ttf.h>

#include "common/Texture.h"
#include "common/Color.h"

#include "World.h"


namespace Brigades {

class Driver {

	public:
		Driver(WorldPtr w);
		void run();

	protected:
		WorldPtr mWorld;
		SDL_Surface* mScreen;
		TTF_Font* mFont;

	private:
		void loadTextures();
		void loadFont();
		boost::shared_ptr<Common::Texture> mSoldierTexture[2][4];
		Common::Color mapSideColor(bool first, const Common::Color& c);
};

typedef boost::shared_ptr<Driver> DriverPtr;

};

#endif

