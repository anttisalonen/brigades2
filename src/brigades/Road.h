#ifndef BRIGADES_ROAD_H
#define BRIGADES_ROAD_H

#include "common/QuadTree.h"
#include "common/LineQuadTree.h"
#include "common/Vector3.h"
#include "common/Vehicle.h"

namespace Brigades {

class Road {
	public:
		Road(const Common::Vector3& start, const Common::Vector3& end);
		const Common::Vector3& getStart() const;
		const Common::Vector3& getEnd() const;

	private:
		Common::Vector3 mStart;
		Common::Vector3 mEnd;
};

}

#endif

