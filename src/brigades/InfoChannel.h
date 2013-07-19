#ifndef BRIGADES_INFOCHANNEL_H
#define BRIGADES_INFOCHANNEL_H

#include <boost/shared_ptr.hpp>

#include "common/Color.h"
#include "common/Rectangle.h"
#include "common/Vector3.h"

namespace Brigades {

class SoldierQuery;

class InfoChannel {
	public:
		static inline boost::shared_ptr<InfoChannel> getInstance();
		static inline void setInstance(boost::shared_ptr<InfoChannel> d);
		virtual ~InfoChannel() { }

		virtual void say(const SoldierQuery& s, const char* msg) = 0;
		virtual void addMessage(const SoldierQuery* s, const Common::Color& c, const char* text) = 0;
};

class DummyInfoChannel : public InfoChannel {
	public:
		virtual void say(const SoldierQuery& s, const char* msg) { }
		virtual void addMessage(const SoldierQuery* s, const Common::Color& c, const char* text) { }
};

extern boost::shared_ptr<InfoChannel> InfoChannelInstance;

boost::shared_ptr<InfoChannel> InfoChannel::getInstance()
{
	if(!InfoChannelInstance) {
		InfoChannelInstance = boost::shared_ptr<InfoChannel>(new DummyInfoChannel());
	}
	return InfoChannelInstance;
}

void InfoChannel::setInstance(boost::shared_ptr<InfoChannel> d)
{
	InfoChannelInstance = d;
}

}

#endif
