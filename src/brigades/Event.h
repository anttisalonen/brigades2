#ifndef BRIGADES_EVENT_H
#define BRIGADES_EVENT_H

#include <boost/shared_ptr.hpp>

namespace Brigades {

enum class EventType {
	Sound,
};

class Event {
	public:
		Event(EventType t, void* data);
		EventType getType() const;
		void* getData();

	private:
		EventType mType;
		void* mData;
};

typedef boost::shared_ptr<Event> EventPtr;

}

#endif

