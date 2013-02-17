#ifndef BRIGADES_SIDE_H
#define BRIGADES_SIDE_H

#include <boost/shared_ptr.hpp>

class Side {
	public:
		Side(bool first);
		bool isFirst() const;
		int getSideNum() const;

	private:
		bool mFirst;
};

typedef boost::shared_ptr<Side> SidePtr;

#endif

