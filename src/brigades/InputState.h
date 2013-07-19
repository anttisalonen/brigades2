#ifndef BRIGADES_INPUTSTATE_H
#define BRIGADES_INPUTSTATE_H

#include <boost/shared_ptr.hpp>

#include "common/Vector3.h"

namespace Brigades {

class SoldierController;
class SoldierAction;

class InputState {
	public:
		InputState();

		// getters
		bool isDigging() const;
		bool isDriving() const;
		bool isShooting() const;
		const Common::Vector3& getPlayerControlVelocity() const;
		Common::Vector3 getMousePositionOnField() const;

		// setters
		void setDigging(bool v);
		void setDriving(bool v);
		void setShooting(bool v);
		void setPlayerControlVelocity(const Common::Vector3& v);
		void setPlayerControlVelocityX(float v);
		void setPlayerControlVelocityY(float v);
		void setMousePositionOnField(const Common::Vector3& v);

	private:
		bool mDigging = false;
		bool mDriving = false;
		bool mShooting = false;
		Common::Vector3 mPvc;
		Common::Vector3 mMousePos;
};

}

#endif

