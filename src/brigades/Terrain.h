#ifndef BRIGADES_TERRAIN_H
#define BRIGADES_TERRAIN_H

#include <boost/shared_ptr.hpp>

#include "common/QuadTree.h"
#include "common/LineQuadTree.h"
#include "common/Vector3.h"
#include "common/Vehicle.h"

#include "Road.h"

namespace Brigades {

class Tree : public Common::Obstacle {
	public:
		Tree(const Common::Vector3& pos, float radius);
};

typedef boost::shared_ptr<Tree> TreePtr;

class Terrain {
	public:
		Terrain(int w, int h);
		std::vector<Tree*> getTreesAt(const Common::Vector3& v, float radius) const;
		std::vector<Road*> getRoadsAt(const Common::Vector3& v, float radius) const;
		float getWidth() const { return mWidth; }
		float getHeight() const { return mHeight; }

	private:
		void addTrees();
		void addRoads();

		float mWidth;
		float mHeight;
		Common::QuadTree<Tree*> mTrees;
		Common::LineQuadTree<Road*> mRoads;
		Common::Vector3 mStart;
		Common::Vector3 mEnd;
		float mRoadRadius;
};

}

#endif

