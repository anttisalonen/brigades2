#include <string.h>

#include "common/Color.h"

#include "Soldier.h"
#include "SoldierController.h"
#include "World.h"
#include "SensorySystem.h"
#include "InfoChannel.h"

#include "ai/SoldierController.h"

using namespace Common;

namespace Brigades {

Soldier::Soldier(boost::shared_ptr<World> w, bool firstside, SoldierRank rank, WarriorType wt)
	: Common::Vehicle(0.5f, 10.0f, 100.0f),
	mWorld(w),
	mSide(w->getSide(firstside)),
	mID(getNextID()),
	mFOV(PI),
	mAlive(true),
	mCurrentWeaponIndex(0),
	mRank(rank),
	mWarriorType(wt),
	mHealth(1.0f),
	mDictator(false),
	mAttacking(false),
	mEnemyContact(false),
	mEnemyContactTimer(1.0f)
{
	mName = generateName();
	if(wt == WarriorType::Vehicle) {
		mRadius = 3.5f;
		mMaxSpeed = 30.0f;
		mMaxAcceleration = 10.0f;
		addWeapon(mWorld->getArmory().getAutomaticCannon());
		addWeapon(mWorld->getArmory().getMachineGun());
		mFOV = TWO_PI;
	} else {
		addWeapon(mWorld->getArmory().getAssaultRifle());
	}
}

void Soldier::init()
{
	setAIController();
	mSensorySystem = SensorySystemPtr(new SensorySystem(shared_from_this()));
}

SidePtr Soldier::getSide() const
{
	return mSide;
}

int Soldier::getID() const
{
	return mID;
}

int Soldier::getSideNum() const
{
	return mSide->getSideNum();
}

int Soldier::getNextID()
{
	static int id = 0;
	return ++id;
}

std::string Soldier::generateName()
{
	static char n = 'A';

	std::string s(1, n);
	if(n == 'Z') {
		n = 'A';
	} else {
		n++;
	}

	return s;
}

void Soldier::update(float time)
{
	if(!time)
		return;

	mSensorySystem->update(time);
	if(mCurrentWeaponIndex < mWeapons.size())
		mWeapons[mCurrentWeaponIndex]->update(time);
	if(mController) {
		mController->act(time);
	}

	mEnemyContactTimer.doCountdown(time);
	if(mEnemyContactTimer.checkAndRewind()) {
		mEnemyContact = false;
		for(auto s : mSensorySystem->getSoldiers()) {
			if(!s->isDead() && s->getSideNum() != getSideNum() &&
					mPosition.distance2(s->getPosition()) <
					mWorld->getVisibility() *
					mWorld->getVisibility()) {
				mEnemyContact = true;
				break;
			}
		}

		if(!mEnemyContact) {
			for(auto s : mCommandees) {
				if(s->hasEnemyContact()) {
					mEnemyContact = true;
					break;
				}
			}
		}
	}

}

float Soldier::getFOV() const
{
	return mFOV;
}

void Soldier::setController(SoldierControllerPtr p)
{
	mController = p;
}

void Soldier::setAIController()
{
	setController(SoldierControllerPtr(new AI::SoldierController(shared_from_this())));
}

SoldierControllerPtr Soldier::getController()
{
	return mController;
}

void Soldier::die()
{
	mAlive = false;
	mSensorySystem->clear();
}

bool Soldier::isDead() const
{
	return !mAlive;
}

bool Soldier::isAlive() const
{
	return mAlive;
}

void Soldier::dig(float time)
{
	mWorld->dig(time, mPosition);
}

void Soldier::clearWeapons()
{
	mWeapons.clear();
}

void Soldier::addWeapon(WeaponPtr w)
{
	mWeapons.push_back(w);
}

WeaponPtr Soldier::getCurrentWeapon()
{
	if(mCurrentWeaponIndex < mWeapons.size())
		return mWeapons[mCurrentWeaponIndex];
	else
		return WeaponPtr();
}

void Soldier::switchWeapon(unsigned int index)
{
	mCurrentWeaponIndex = index;
}

const std::vector<WeaponPtr>& Soldier::getWeapons() const
{
	return mWeapons;
}

const WorldPtr Soldier::getWorld() const
{
	return mWorld;
}

WorldPtr Soldier::getWorld()
{
	return mWorld;
}

Common::Vector3 Soldier::getUnitPosition() const
{
	if(mCommandees.size() == 0) {
		return getPosition();
	} else {
		Vector3 midp;
		for(auto& c : mCommandees) {
			midp += c->getUnitPosition();
		}
		midp /= mCommandees.size();
		return midp;
	}
}

SensorySystemPtr Soldier::getSensorySystem()
{
	return mSensorySystem;
}

std::set<SoldierPtr> Soldier::getKnownEnemySoldiers() const
{
	std::set<SoldierPtr> ret;
	std::vector<SoldierPtr> mySeen = mSensorySystem->getSoldiers();
	ret.insert(mySeen.begin(), mySeen.end());
	for(auto& c : mCommandees) {
		auto res = c->getKnownEnemySoldiers();
		ret.insert(res.begin(), res.end());
	}
	return ret;
}

void Soldier::addEvent(EventPtr e)
{
	mEvents.push_back(e);
}

std::vector<EventPtr>& Soldier::getEvents()
{
	return mEvents;
}

bool Soldier::handleEvents()
{
	bool ret = !mEvents.empty();

	for(auto e : mEvents) {
		e->handleEvent(shared_from_this());
	}

	mEvents.clear();

	return ret;
}

SoldierRank Soldier::getRank() const
{
	return mRank;
}

void Soldier::setRank(SoldierRank r)
{
	mRank = r;
}

void Soldier::addCommandee(SoldierPtr s)
{
	if(!isDead() && !s->isDead()) {
		mCommandees.push_back(s);
		s->setLeader(shared_from_this());
	}
}

void Soldier::removeCommandee(SoldierPtr s)
{
	mCommandees.remove(s);
	s->setLeader(SoldierPtr());
}

std::list<SoldierPtr>& Soldier::getCommandees()
{
	return mCommandees;
}

void Soldier::setLeader(SoldierPtr s)
{
	mLeader = s;
}

SoldierPtr Soldier::getLeader()
{
	return mLeader;
}

bool Soldier::seesSoldier(const SoldierPtr s)
{
	for(auto ss : mSensorySystem->getSoldiers()) {
		if(s == ss) {
			return true;
		}
	}

	return false;
}

void Soldier::setFormationOffset(const Vector3& v)
{
	mDefendPosition = Vector3();
	mFormationOffset = v;
}

void Soldier::setDefendPosition(const Common::Vector3& v)
{
	mFormationOffset = Vector3();
	mDefendPosition = v;
}

const Vector3& Soldier::getFormationOffset() const
{
	return mFormationOffset;
}

const Vector3& Soldier::getDefendPosition() const
{
	return mDefendPosition;
}

void Soldier::setColumnFormation(float dist)
{
	int i = 1;
	for(auto c : mCommandees) {
		float pos = i * -dist;
		c->setFormationOffset(Vector3(pos, 0.0f, 0.0f));
		i++;
	}
}

void Soldier::setLineFormation(float dist)
{
	int i = 2;
	for(auto c : mCommandees) {
		float pos = (i / 2) * dist * (i & 1 ? 1.0f : -1.0f);
		c->setFormationOffset(Vector3(0.0f, pos, 0.0f));
		i++;
	}
}

void Soldier::pruneCommandees()
{
	auto sit = mCommandees.begin();
	while(sit != mCommandees.end()) {
		if((*sit)->isDead()) {
			sit = mCommandees.erase(sit);
		}
		else {
			++sit;
		}
	}

}

WarriorType Soldier::getWarriorType() const
{
	return mWarriorType;
}

void Soldier::reduceHealth(float n)
{
	mHealth -= n;
}

float Soldier::getHealth() const
{
	return mHealth;
}

float Soldier::damageFactorFromWeapon(const WeaponPtr w) const
{
	if(getWarriorType() == WarriorType::Soldier) {
		return w->getDamageAgainstSoftTargets();
	} else {
		return w->getDamageAgainstLightArmor();
	}
}

bool Soldier::hasWeaponType(const char* wname) const
{
	for(auto w : mWeapons) {
		if(!strcmp(wname, w->getName()))
			return true;
	}
	return false;
}

void Soldier::setDictator(bool d)
{
	mDictator = d;
	if(mDictator) {
		mMaxSpeed = 0.0f;
		mMaxAcceleration = 0.0f;
	}
}

bool Soldier::isDictator() const
{
	return mDictator;
}

bool Soldier::canCommunicateWith(const SoldierPtr p) const
{
	return !isDead() && !p->isDead() && ((hasRadio() && p->hasRadio()) ||
			(Entity::distanceBetween(*this, *p) < 10.0f));
}

bool Soldier::hasRadio() const
{
	return true;
}

bool Soldier::hasEnemyContact() const
{
	return mEnemyContact;
}

const std::string& Soldier::getName() const
{
	return mName;
}

const char* Soldier::rankToString(SoldierRank r)
{
	switch(r) {
		case SoldierRank::Private:
			return "Private";
		case SoldierRank::Sergeant:
			return "Sergeant";
		case SoldierRank::Lieutenant:
			return "Lieutenant";
		case SoldierRank::Captain:
			return "Captain";
	}
}

bool Soldier::defending() const
{
	return !mAttacking;
}

void Soldier::setDefending()
{
	mAttacking = false;
}

bool Soldier::giveAttackOrder(const AttackOrder& r)
{
	// set mAttackOrder for handleAttackOrder(), then set it back in case of failure
	mAttacking = true;
	auto old = mAttackOrder;
	mAttackOrder = r;
	if(!mController->handleAttackOrder(r)) {
		std::cout << "Warning: controller couldn't handle attack order\n";
		mAttacking = false;
		mAttackOrder = old;
		return false;
	} else {
		return true;
	}
}

const AttackOrder& Soldier::getAttackOrder() const
{
	return mAttackOrder;
}

const Common::Vector3& Soldier::getCenterOfAttackArea() const
{
	return mAttackOrder.CenterPoint;
}

void Soldier::globalMessage(const char* s)
{
	char buf[256];
	snprintf(buf, 255, "%s %s: %s", rankToString(mRank), mName.c_str(), s);
	buf[255] = 0;
	InfoChannel::getInstance()->addMessage(shared_from_this(), Common::Color::White, buf);
}

bool Soldier::reportSuccessfulAttack()
{
	if(mRank >= SoldierRank::Lieutenant) {
		char buf[256];
		buf[255] = 0;
		snprintf(buf, 255, "I'm reporting a successful attack to %s %s",
				rankToString(mLeader->getRank()),
				mLeader->getName().c_str());
		globalMessage(buf);
	}
	return mLeader->successfulAttackReported(getAttackOrder());
}

bool Soldier::successfulAttackReported(const AttackOrder& r)
{
	if(!mController->handleAttackSuccess(shared_from_this(), r)) {
		std::cerr << "Warning: controller couldn't handle successful attack\n";
		return false;
	} else {
		return true;
	}
}

}
