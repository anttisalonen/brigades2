#include "InputState.h"

namespace Brigades {

InputState::InputState()
{
}


// getters
bool InputState::isDigging() const
{
	return mDigging;
}

bool InputState::isShooting() const
{
	return mShooting;
}

const Common::Vector3& InputState::getPlayerControlVelocity() const
{
	return mPvc;
}

Common::Vector3 InputState::getMousePositionOnField() const
{
	return mMousePos;
}


// setters
void InputState::setDigging(bool v)
{
	mDigging = v;
}

void InputState::setShooting(bool v)
{
	mShooting = v;
}

void InputState::setPlayerControlVelocity(const Common::Vector3& v)
{
	mPvc = v;
}

void InputState::setPlayerControlVelocityX(float v)
{
	mPvc.x = v;
}

void InputState::setPlayerControlVelocityY(float v)
{
	mPvc.y = v;
}

void InputState::setMousePositionOnField(const Common::Vector3& v)
{
	mMousePos = v;
}

}

