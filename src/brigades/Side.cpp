#include "Side.h"

Side::Side(bool first)
	: mFirst(first)
{
}

bool Side::isFirst() const
{
	return mFirst;
}

int Side::getSideNum() const
{
	return mFirst ? 0 : 1;
}


