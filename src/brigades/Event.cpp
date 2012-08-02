#include "Event.h"

namespace Brigades {

Event::Event(EventType t, void* data)
	: mType(t),
	mData(data)
{
}

EventType Event::getType() const
{
	return mType;
}

void* Event::getData()
{
	return mData;
}

}

