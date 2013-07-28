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
		void setDriverOccupied(bool o);
		bool driverOccupied() const;
		void addPassenger();
		void removePassenger();
		int freePassengerSeats() const;
		bool roadCheck(float time);

	private:
		static int getNextID();

		int mID;
		int mSide;
		float mHealth = 1.0f;
		bool mOccupied = false;
		int mFreeSeats = 8;
		Common::SteadyTimer mRoadCheck;
};

typedef boost::shared_ptr<Armor> ArmorPtr;

class ArmorQuery {
	public:
		ArmorQuery(const ArmorPtr p);
		bool queryIsValid() const;
		const Common::Vector3& getPosition() const;
		int getSideNum() const;
		bool isDestroyed() const;
		float getHealth() const;
		float damageFactorFromWeapon(const WeaponPtr w) const;
		float getXYRotation() const;
		Common::Vector3 getHeadingVector() const;
		Common::Vector3 getVelocity() const;
		bool driverOccupied() const;
		int freePassengerSeats() const;

		bool operator==(const ArmorQuery& f) const;
		bool operator!=(const ArmorQuery& f) const;
		bool operator<(const ArmorQuery& f) const;

	private:
		const ArmorPtr mArmor;
		friend ArmorQuery;
		friend class SoldierAction;
};

}

#endif

