#ifndef BRIGADES_ARMORY_H
#define BRIGADES_ARMORY_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "common/Vector3.h"
#include "common/Clock.h"

namespace Brigades {

class Soldier;
typedef boost::shared_ptr<Soldier> SoldierPtr;
class World;
typedef boost::shared_ptr<World> WorldPtr;

class Weapon : public boost::enable_shared_from_this<Weapon> {
	public:
		Weapon(const char* name, float range, float velocity, float loadtime,
				float variation,
				float softd = 1.0f,
				float lightd = 0.0f,
				float heavyd = 0.0f);
		virtual ~Weapon() { }
		void update(float time);
		bool canShoot() const;
		float getRange() const;
		float getVelocity() const;
		float getLoadTime() const;
		float getVariation() const;
		void shoot(WorldPtr w, const SoldierPtr s, const Common::Vector3& dir);
		float getDamageAgainstSoftTargets() const;
		float getDamageAgainstLightArmor() const;
		float getDamageAgainstHeavyArmor() const;
		const char* getName() const;

	protected:
		const char* mName;
		float mRange;
		float mVelocity;
		Common::Countdown mLoadTime;
		float mVariation;
		float mSoftDamage;
		float mLightArmorDamage;
		float mHeavyArmorDamage;
};

typedef boost::shared_ptr<Weapon> WeaponPtr;

class Armory {
	public:
		virtual WeaponPtr getAssaultRifle() = 0;
		virtual WeaponPtr getMachineGun() = 0;
		virtual WeaponPtr getBazooka() = 0;
		virtual WeaponPtr getPistol() = 0;
		virtual WeaponPtr getAutomaticCannon() = 0;
		virtual ~Armory() { }
		Armory* getInstance();
		void setInstance(Armory* a);

	private:
		Armory* mInstance;
};

class AssaultRifle : public Weapon {
	public:
		AssaultRifle();
		const char* getName() const;
};

class MachineGun : public Weapon {
	public:
		MachineGun();
		const char* getName() const;
};

class Bazooka : public Weapon {
	public:
		Bazooka();
		const char* getName() const;
};

class Pistol : public Weapon {
	public:
		Pistol();
		const char* getName() const;
};

class AutomaticCannon : public Weapon {
	public:
		AutomaticCannon();
		const char* getName() const;
};

}

#endif

