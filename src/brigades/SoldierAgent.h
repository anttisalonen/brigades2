#ifndef BRIGADES_SOLDIERAGENT_H
#define BRIGADES_SOLDIERAGENT_H

#include <boost/shared_ptr.hpp>

#include "SoldierQuery.h"
#include "SoldierAction.h"

namespace Brigades {

class SoldierController;

class SoldierAgent {
	public:
		SoldierAgent(const boost::shared_ptr<SoldierController> s);
		void updateController(float time);
		virtual std::vector<SoldierAction> update(float time) = 0;

	protected:
		SoldierQuery getControlledSoldier() const;
		Common::Vector3 createMovement(bool defmov, const Common::Vector3& mov) const;

	private:
		const boost::shared_ptr<SoldierController> mController;
};

}

#endif

