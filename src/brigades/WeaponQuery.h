#ifndef BRIGADES_WEAPONQUERY_H
#define BRIGADES_WEAPONQUERY_H

#include <boost/shared_ptr.hpp>

#include "Armory.h"

namespace Brigades {

class WeaponQuery;
typedef boost::shared_ptr<WeaponQuery> WeaponQueryPtr;

class WeaponQuery {
	public:
		WeaponQuery(const WeaponPtr s);
		bool queryIsValid() const;
		WeaponType getWeaponType() const;
		bool canShoot() const;
		float getRange() const;
		float getVelocity() const;
		float getLoadTime() const;
		float getVariation() const;
		float getDamageAgainstSoftTargets() const;
		float getDamageAgainstLightArmor() const;
		float getDamageAgainstHeavyArmor() const;
		const char* getName() const;
		bool speedVariates() const;

		bool operator==(const WeaponQuery& f) const;
		bool operator!=(const WeaponQuery& f) const;

	private:
		const WeaponPtr mWeapon;
};

}

#endif

