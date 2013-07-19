#ifndef BRIGADES_ARMOR_H
#define BRIGADES_ARMOR_H

#include <boost/shared_ptr.hpp>

#include "common/Vehicle.h"

#include "Armory.h"

namespace Brigades {

class Armor : public Common::Vehicle {
	public:
		Armor(int sidenum);
		int getID() const;
		int getSideNum() const;
		bool isDestroyed() const;
		void destroy();
		void reduceHealth(float n);
		float getHealth() const;
		float damageFactorFromWeapon(const WeaponPtr w) const;

	private:
		static int getNextID();

		bool mDestroyed;
		int mID;
		int mSide;
		float mHealth = 1.0f;
};

typedef boost::shared_ptr<Armor> ArmorPtr;

}

#endif

