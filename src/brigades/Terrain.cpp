#include "Terrain.h"

#include "common/Vector2.h"
#include "common/Random.h"
#include "common/AStar.h"

using namespace Common;

namespace Brigades {

Tree::Tree(const Vector3& pos, float radius)
	: Obstacle(radius)
{
	mPosition = pos;
}


Terrain::Terrain(int w, int h)
	: mWidth(w),
	mHeight(h),
	mTrees(AABB(Vector2(0, 0), Vector2(w * 0.5f, h * 0.5f))),
	mRoads(Common::LineQuadTree<Road*>(AABB(Vector2(0, 0), Vector2(w * 0.5f, h * 0.5f)))),
	mRoadRadius(5.0f)
{
	addTrees();
	addRoads();
}

void Terrain::addTrees()
{
	static const int squareSide = 64;
	int numXSquares = mWidth / squareSide;
	int numYSquares = mHeight / squareSide;

	for(int k = -numYSquares / 2; k < numYSquares / 2; k++) {
		for(int j = -numXSquares / 2; j < numXSquares / 2; j++) {
			if((k == -numYSquares / 2 && j == -numXSquares / 2) ||
				       (k == numYSquares / 2 - 1 && j == numXSquares / 2 -1))
				continue;

			int treefactor = 10 + Random::uniform() * 10;
			for(int i = 0; i < treefactor; i++) {
				float x = Random::uniform();
				float y = Random::uniform();
				float r = Random::uniform();

				const float maxRadius = 8.0f;

				x *= squareSide;
				y *= squareSide;
				x += j * squareSide;
				y += k * squareSide;
				r = Common::clamp(2.0f, r * maxRadius, maxRadius);

				bool tooclose = false;
				for(auto t : mTrees.query(AABB(Vector2(x, y), Vector2(maxRadius * 2.0f, maxRadius * 2.0f)))) {
					float maxdist = r + t->getRadius();
					if(Vector3(x, y, 0.0f).distance2(t->getPosition()) <
							maxdist * maxdist) {
						tooclose = true;
						break;
					}
				}
				if(tooclose) {
					continue;
				}

				// we're leaking the trees for now.
				Tree* tree = new Tree(Vector3(x, y, 0), r);
				bool ret = mTrees.insert(tree, Vector2(x, y));
				if(!ret) {
					std::cout << "Error: couldn't add tree at " << x << ", " << y << "\n";
					assert(0);
				}
			}
		}
	}
	std::cout << "Added " << mTrees.size() << " trees.\n";
}

struct GraphNode {
	Vector2 location;
	std::set<GraphNode*> neighbours;

	bool operator<(const GraphNode& f) const;
	bool operator==(const GraphNode& f) const;
};

bool GraphNode::operator<(const GraphNode& f) const
{
	return location < f.location;
}

bool GraphNode::operator==(const GraphNode& f) const
{
	return location == f.location;
}

class RoadSolver {
	public:
		void setGoalNode(const GraphNode* gl) { mGoal = gl; }
		std::set<GraphNode> getNeighbours(const GraphNode& a);
		int cost(const GraphNode& a, const GraphNode& b);
		int heuristic(const GraphNode& a);
		bool goal(const GraphNode& a);
		void addRoad(const GraphNode& a, const GraphNode& b);
		const std::map<Vector2, std::set<Vector2>>& getRoads() const { return mRoads; }

		std::list<GraphNode> solve(const GraphNode& start);

	private:
		const GraphNode* mGoal = nullptr;
		std::map<Vector2, std::set<Vector2>> mRoads;
};

std::set<GraphNode> RoadSolver::getNeighbours(const GraphNode& a)
{
	std::set<GraphNode> ret;
	for(auto n : a.neighbours)
		ret.insert(*n);

	return ret;
}

int RoadSolver::cost(const GraphNode& a, const GraphNode& b)
{
	auto it = mRoads.find(a.location);
	if(it != mRoads.end()) {
		auto st = it->second;
		if(st.find(b.location) != st.end()) {
			return 1;
		}
	}
	float dist = a.location.distance(b.location);
	return 10 + sqrt(dist);
}

int RoadSolver::heuristic(const GraphNode& a)
{
	return 10 + sqrt(mGoal->location.distance(a.location));
}

bool RoadSolver::goal(const GraphNode& a)
{
	return a.location == mGoal->location;
}

void RoadSolver::addRoad(const GraphNode& a, const GraphNode& b)
{
	mRoads[a.location].insert(b.location);
	mRoads[b.location].insert(a.location);
}

std::list<GraphNode> RoadSolver::solve(const GraphNode& start)
{
	return AStar<GraphNode>::solve([&](const GraphNode& a) { return getNeighbours(a); },
		       [&](const GraphNode& a, const GraphNode& b) { return cost(a, b); },
		       [&](const GraphNode& a) { return heuristic(a); },
		       [&](const GraphNode& a) { return goal(a); },
		       start);
}

void Terrain::addRoads()
{
	printf("Creating roads...\n");
	Common::QuadTree<GraphNode*> nodes(AABB(Vector2(0, 0), Vector2(mWidth * 0.5f, mHeight * 0.5f)));

	static const int squareSide = 64;
	int numXSquares = mWidth / squareSide;
	int numYSquares = mHeight / squareSide;
	int numNodes = 0;

	// create nodes
	for(int k = -numYSquares / 2; k < numYSquares / 2; k++) {
		for(int j = -numXSquares / 2; j < numXSquares / 2; j++) {
			for(int i = 0; i < 3; i++) {
				float x = Random::uniform();
				float y = Random::uniform();

				x *= squareSide;
				y *= squareSide;
				x += j * squareSide;
				y += k * squareSide;

				GraphNode* n = new GraphNode;
				n->location = Vector2(x, y);
				bool ret = nodes.insert(n, n->location);
				assert(ret);
				numNodes++;
			}
		}
	}

	// find nodes' neighbours
	int totalNodes = 0;
	for(auto it = nodes.begin(); it != nodes.end(); ++it) {
		totalNodes++;
		const auto& mypos = (*it)->location;
		if((*it)->neighbours.size() >= 20)
			continue;

		auto possibleNeighbours = nodes.query(AABB(mypos, Vector2(squareSide * 0.5f, squareSide * 0.5f)));
		if(possibleNeighbours.size() < 3) {
			possibleNeighbours = nodes.query(AABB(mypos, Vector2(squareSide, squareSide)));
			if(possibleNeighbours.size() < 2) {
				possibleNeighbours = nodes.query(AABB(mypos, Vector2(squareSide * 2.0f, squareSide * 2.0f)));
			}
		}

		for(const auto& n : possibleNeighbours) {
			(*it)->neighbours.insert(n);
			n->neighbours.insert(*it);
			if((*it)->neighbours.size() >= 20)
				break;
		}
	}
	printf("Visited %d out of %d nodes.\n", totalNodes, nodes.size());
	assert(totalNodes == nodes.size());

	// assign significant nodes (road endpoints)
	int numSignificantNodes = numNodes / 30;
	assert(numNodes);
	assert(numSignificantNodes);

	struct SignificantNode {
		GraphNode* graphnode;
		std::set<SignificantNode*> neighbours;
	};
	std::vector<SignificantNode*> sigNodes;

	int nodesLeft = numNodes;
	for(auto it = nodes.begin(); it != nodes.end(); ++it) {
		assert(nodesLeft);
		float prob = numSignificantNodes / (float)nodesLeft;
		nodesLeft--;
		bool have = Random::uniform() < prob;
		if(have) {
			SignificantNode* snode = new SignificantNode;
			snode->graphnode = *it;
			sigNodes.push_back(snode);
			numSignificantNodes--;
			if(numSignificantNodes <= 0) {
				break;
			}
		}
	}

	printf("Have %zd significant nodes.\n", sigNodes.size());

	// find significant nodes' neighbours (roads)
	for(auto snode : sigNodes) {
		if(snode->neighbours.size() >= 6)
			break;

		auto others = sigNodes;

		std::sort(others.begin(), others.end(), [&](const SignificantNode* n1, const SignificantNode* n2) {
				return snode->graphnode->location.distance(n1->graphnode->location) <
					snode->graphnode->location.distance(n2->graphnode->location);
			});

		for(auto& othnode : others) {
			if(snode->neighbours.size() >= 6)
				break;

			if(othnode == snode)
				continue;

			snode->neighbours.insert(othnode);
			othnode->neighbours.insert(snode);
		}
	}

	// create roads
	assert(sigNodes.size() > 1);
	RoadSolver r;
	for(auto snode : sigNodes) {
		for(auto snode2 : snode->neighbours) {
			assert(snode != snode2);
			r.setGoalNode(snode2->graphnode);
			auto path = r.solve(*snode->graphnode);
			if(path.size() > 1) {
				auto prev = path.begin();
				for(auto it = std::next(prev); it != path.end(); ++it) {
					r.addRoad(*prev, *it);
					prev = it;
				}
			}
		}
	}

	// add roads to terrain
	int removedTrees = 0;
	for(const auto& road : r.getRoads()) {
		const auto& s1 = road.first;
		for(const auto& s2 : road.second) {
			auto s13 = Vector3(s1.x, s1.y, 0.0f);
			auto s23 = Vector3(s2.x, s2.y, 0.0f);

			// do not add duplicate roads
			if(s13 < s23) {
				auto midpoint = (s1 + s2) * 0.5f;
				auto trees = getTreesAt(Vector3(midpoint.x, midpoint.y, 0.0f), midpoint.distance(s2));
				for(auto& t : trees) {
					if(Math::segmentCircleIntersect(s13, s23,
								t->getPosition(), t->getRadius() + mRoadRadius)) {
						bool succ = mTrees.deleteT(t, Vector2(t->getPosition().x, t->getPosition().y));
						assert(succ);
						removedTrees++;
					}
				}

				// leaking roads for now
				auto robj = new Road(s13, s23);
				bool succ = mRoads.insert(robj, AABB(midpoint, Vector2(fabs(midpoint.x - s2.x),
								fabs(midpoint.y - s2.y))));
				assert(succ);
			}
		}
	}

	printf("Removed %d trees.\n", removedTrees);
	printf("Have %zd road segments.\n", r.getRoads().size());

	// clean up
	for(auto n : sigNodes) {
		delete n;
	}

	for(auto it = nodes.begin(); it != nodes.end(); ++it) {
		delete *it;
	}
	nodes.clear();

	printf("Done creating roads.\n");
}

std::vector<Tree*> Terrain::getTreesAt(const Vector3& v, float radius) const
{
	return mTrees.query(AABB(Vector2(v.x, v.y), Vector2(radius, radius)));
}

std::vector<Road*> Terrain::getRoadsAt(const Vector3& v, float radius) const
{
	return mRoads.query(AABB(Vector2(v.x, v.y), Vector2(radius, radius)));
}

}

