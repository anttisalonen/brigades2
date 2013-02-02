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

class WeaponType {
	public:
		WeaponType(const char* name, float range, float velocity, float loadtime,
				float variation,
				bool speedVariate,
				float softd = 1.0f,
				float lightd = 0.0f,
				float heavyd = 0.0f);
		virtual ~WeaponType() { }
		float getRange() const;
		float getVelocity() const;
		float getLoadTime() const;
		float getVariation() const;
		float getDamageAgainstSoftTargets() const;
		float getDamageAgainstLightArmor() const;
		float getDamageAgainstHeavyArmor() const;
		const char* getName() const;
		bool speedVariates() const;

	protected:
		const char* mName;
		float mRange;
		float mVelocity;
		float mLoadTime;
		float mVariation;
		bool mSpeedVariates;
		float mSoftDamage;
		float mLightArmorDamage;
		float mHeavyArmorDamage;
};

class Weapon : public boost::enable_shared_from_this<Weapon> {
	public:
		Weapon(boost::shared_ptr<WeaponType> type);
		virtual ~Weapon() { }
		void update(float time);
		boost::shared_ptr<WeaponType> getWeaponType() const;
		bool canShoot() const;
		void shoot(WorldPtr w, const SoldierPtr s, const Common::Vector3& dir);

		float getRange() const;
		float getVelocity() const;
		float getLoadTime() const;
		float getVariation() const;
		float getDamageAgainstSoftTargets() const;
		float getDamageAgainstLightArmor() const;
		float getDamageAgainstHeavyArmor() const;
		const char* getName() const;

	protected:
		boost::shared_ptr<WeaponType> mWeapon;
		Common::Countdown mLoading;
};

typedef boost::shared_ptr<Weapon> WeaponPtr;

class Armory {
	public:
		WeaponPtr getAssaultRifle();
		WeaponPtr getMachineGun();
		WeaponPtr getBazooka();
		WeaponPtr getPistol();
		WeaponPtr getAutomaticCannon();
		virtual ~Armory() { }
		Armory* getInstance();
		void setInstance(Armory* a);

	protected:
		boost::shared_ptr<WeaponType> mAssaultRifle;
		boost::shared_ptr<WeaponType> mMachineGun;
		boost::shared_ptr<WeaponType> mBazooka;
		boost::shared_ptr<WeaponType> mPistol;
		boost::shared_ptr<WeaponType> mAutoCannon;

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

