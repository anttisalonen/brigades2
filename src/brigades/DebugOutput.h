#ifndef BRIGADES_DEBUGOUTPUT_H
#define BRIGADES_DEBUGOUTPUT_H

#include <boost/shared_ptr.hpp>

#include "common/Color.h"
#include "common/Rectangle.h"
#include "common/Vector3.h"

namespace Brigades {

class DebugOutput {
	public:
		static inline boost::shared_ptr<DebugOutput> getInstance();
		static inline void setInstance(boost::shared_ptr<DebugOutput> d);
		virtual ~DebugOutput() { }

		virtual void markArea(const Common::Color& c, const Common::Rectangle& r, bool onlyframes) = 0;
		virtual void addArrow(const Common::Color& c, const Common::Vector3& start, const Common::Vector3& end) = 0;
};

class DummyDebugOutput : public DebugOutput {
	public:
		virtual void markArea(const Common::Color& c, const Common::Rectangle& r, bool onlyframes) { }
		virtual void addArrow(const Common::Color& c, const Common::Vector3& start, const Common::Vector3& end) { }
};

extern boost::shared_ptr<DebugOutput> DebugOutputInstance;

boost::shared_ptr<DebugOutput> DebugOutput::getInstance()
{
	if(!DebugOutputInstance) {
		DebugOutputInstance = boost::shared_ptr<DebugOutput>(new DummyDebugOutput());
	}
	return DebugOutputInstance;
}

void DebugOutput::setInstance(boost::shared_ptr<DebugOutput> d)
{
	DebugOutputInstance = d;
}

}

#endif
