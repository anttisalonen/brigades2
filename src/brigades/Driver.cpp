#include "Driver.h"

#include "common/SDL_utils.h"

using namespace Common;

namespace Brigades {

static int screenWidth = 800;
static int screenHeight = 600;

Driver::Driver(WorldPtr w)
	: mWorld(w)
{
	mScreen = SDL_utils::initSDL(screenWidth, screenHeight);

	loadTextures();
	loadFont();
}

void Driver::run()
{
	std::cout << "Hello world!\n";
}

void Driver::loadTextures()
{
	SDLSurface surfs[] = { SDLSurface("share/soldier1-n.png"),
		SDLSurface("share/soldier1-w.png"),
		SDLSurface("share/soldier1-s.png"),
		SDLSurface("share/soldier1-e.png"),
		};

	int i = 0;
	for(auto& s : surfs) {
		for(int j = 0; j < 2; j++) {
			SDLSurface surf(s);
			surf.mapPixelColor( [&] (const Color& c) { return mapSideColor(j == 0, c); } );
			mSoldierTexture[j][i] = boost::shared_ptr<Texture>(new Texture(surf, 0, 32));
		}
		i++;
	}
}

void Driver::loadFont()
{
	mFont = TTF_OpenFont("share/DejaVuSans.ttf", 12);
	if(!mFont) {
		fprintf(stderr, "Could not open font: %s\n", TTF_GetError());
		throw std::runtime_error("Loading font");
	}
}

Common::Color Driver::mapSideColor(bool first, const Common::Color& c)
{
	if(c.r == 255 && c.b == 255) {
		if(first) {
			return Common::Color::Red;
		} else {
			return Common::Color::Blue;
		}
	}

	return c;
}

}
