#include "Road.h"

#include "common/Vector2.h"


using namespace Common;

namespace Brigades {

Road::Road(const Common::Vector3& start, const Common::Vector3& end)
	: mStart(start),
	mEnd(end)
{
}

const Common::Vector3& Road::getStart() const
{
	return mStart;
}

const Common::Vector3& Road::getEnd() const
{
	return mEnd;
}

}

