/**
 * COMP6771 Assignment 3
 * Michael Su - z3466178
 *
 * Submission for ass3 - a generic directed value-like graph container library
 */
#ifndef GRAPH_H_GUARD
#define GRAPH_H_GUARD

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>

namespace cs6771 {

	// templated function to compare for equality of node values and edge weights
	template <typename T>
	bool equals(const T& a, const T& b) {
		return (!(a < b) && !(b < a));
	}

	// @TODO const correctness for all functions
	// @TODO copy and MOVE!!! constructors
	// @TODO go through everything and check every == for equals()
	// @TODO check all functions and exceptions thrown
	// Graph class declaration
	template <typename N, typename E>
	class Graph {
	public:
		// default constructor
		Graph();
		// add a new node to the graph, returns true if node is added, and false if it already exists
		bool addNode(const N& n);
		// add a new edge to the given node, returns true if added, false if it already exists
		bool addEdge(const N& src, const N& dest, const E& weight);
		// replaces the data stored at the node with the given value
		bool replace(const N& replace, const N& with);
		// replaces data stored at node with data stored on another node on the graph
		// first node param is the node that is destroyed. if either node not found, runtime_error thrown
		// edges of both nodes retained after merge, except edges between the two nodes themselves
		// @TODO: double check there are no duplicate edges in the merged node
		void mergeReplace(const N& destroy, const N& second);
		// deletes node with given value, as well as all edges connected to and from it
		void deleteNode(const N& del) noexcept;
		// deletes an edge between two nodes with a given weight. doesn't throw exceptions
		void deleteEdge(const N& src, const N& dest, const E& weight) noexcept;

		// checks if a node already exists
		bool isNode(const N& n);
		// checks if there is an edge from the first node to the second
		// if either node is not found, std::runtime_error is thrown
		bool isConnected(const N& a, const N& b);
		// prints out all nodes in this graph
		void printNodes();
		// prints all edges of the node with the given value, sorted by edge cost incrementing
		// if edge costs are equivalent, sort by < on dest node's value
		void printEdges(const N& n);


	private:
		// GraphEdge prototype
		class GraphEdge;

		// @TODO figure out where to use smart pointers in the nested classes (and other places?)
		// nested class Node: contains a list of edges, and the node value itself
		class GraphNode {
		public:
			// these should be okay with compiler-auto-generated constructors and destructor
			std::vector<std::shared_ptr<GraphEdge>> edges;
			N value;

			// sorts all the edges in this node into the correct order
			void sortEdges() {
				// first, get rid of all edges that have destNode (weak_ptr) expired
				auto i = edges.begin();
				while (i != edges.end()) {
					// std::cout << typeid(*i).name() << std::endl;
					if (!((*i)->destNode.lock())) { // weak_ptr expired
						edges.erase(i);
					} else {
						++i;
					}
				}
				// then sort

				std::sort(edges.begin(), edges.end(),
					[](std::shared_ptr<GraphEdge> a, std::shared_ptr<GraphEdge> b) {
						if (equals(a->weight, b->weight)) {
							// can convert destNode without checking because of loop just above
							return a->destNode.lock()->value < b->destNode.lock()->value;
						}
						return a->weight < b->weight; // edge order is increasing weight
					});
			}

			// checks if there is any edge (regardless of weight) to the node with the given value
			bool hasEdgeTo(const N& to) {
				for (auto i = edges.begin(); i != edges.end(); ++i) {
					if (auto sptrDest = (*i)->destNode.lock()) { // if weak ptr is still alive
						if (equals(sptrDest->value, to)) {
							return true;
						}
					}
				}
				return false;
			}

			// deletes all edges from this node to the node with the given value
			// @TODO figure out if shared_ptr<GraphEdge> is leaking anywhere!!!
			void deleteEdgesTo(const N& to) {
				auto i = edges.begin();
				while (i != edges.end()) {
					bool deleted = false;
					if (auto sptrDest = (*i)->destNode.lock()) { // if weak ptr is still alive
						if (equals(sptrDest->value, to)) { // this edge should be deleted
							deleted = true;
							edges.erase(i); // will call the smart pointer's destructor
						}
					}
					if (!deleted) {
						++i;
					}
				}
			}

			// takes all of this Node's edges and gives them to the Node that's supplied (unless duplicate)
			// assumes that deleteEdgesTo has been called between both of the two nodes
			void giveEdgesTo(std::shared_ptr<GraphNode> to) {
				// copy shared_ptr over from this->edges to to->edges
				for (auto i = edges.begin(); i != edges.end(); ++i) {
					// check if to->edges already contains a duplicate of this edge
					bool exists = false;
					for (auto t = to->edges.begin(); t != to->edges.end(); ++t) {
						auto ilock = (*i)->destNode.lock();
						auto tlock = (*t)->destNode.lock();
						if (!ilock || !tlock) {
							continue;
						}
						// if weight and dest node are both equivalent, identical edge found
						if (equals((*t)->weight, (*i)->weight) && equals(tlock->value, ilock->value)) {
							exists = true;
							break;
						}
					}
					if (!exists) {
						to->edges.push_back(*i); // copy over shared_ptr<GraphEdge> if no duplicate found
					}
				}
				// clear this->edges which will call destructors on the smart pointers contained
				edges.clear();
			}
		};

		// nested class Edge: contains the N value of the dest node, and the edge weight
		class GraphEdge {
		public:
			E weight;
			std::weak_ptr<GraphNode> destNode;
		};

		// collection of smart pointers corresponding to the Nodes in this graph
		// @TODO: figure out if these should be unique_ptr or shared_ptr!
		std::vector<std::shared_ptr<GraphNode>> nodes;

		// sort Nodes into correct order
		// @TODO: implement mutable sort flag to prevent unnecessary sorting
		void sortNodes() {
			std::sort(nodes.begin(), nodes.end(),
				[](std::shared_ptr<GraphNode> a, std::shared_ptr<GraphNode> b) {
					int sizeA = a->edges.size();
					int sizeB = b->edges.size();
					if (sizeA == sizeB) {
						return a->value < b->value;
					}
					return sizeB < sizeA; // node order is most edges -> least edges
				});
		}

		// gets the GraphNode with the given value
		std::shared_ptr<GraphNode> getNode(const N& n) {
			for (auto i = nodes.begin(); i != nodes.end(); ++i) {
				if (equals((*i)->value, n)) {
					return *i;
				}
			}
			// throw exception if not found
			throw std::runtime_error("node not found");
		}
	};

	// default constructor definition
	template <typename N, typename E>
	Graph<N, E>::Graph() {
		// do nothing as of now
	}

	// add a new node to the graph
	template <typename N, typename E>
	bool Graph<N, E>::addNode(const N& n) {
		if (!isNode(n)) { // doesn't exist
			std::shared_ptr<GraphNode> node = std::make_shared<GraphNode>();
			node->value = n;
			nodes.push_back(std::move(node));
			return true;
		}
		return false; // already exists so false
	}

	// add a new edge to the given node
	template <typename N, typename E>
	bool Graph<N, E>::addEdge(const N& src, const N& dest, const E& weight) {
		std::shared_ptr<GraphNode> srcNode = getNode(src);
		std::shared_ptr<GraphNode> destNode = getNode(dest);
		// getNode throws std::runtime_error if src or dest aren't found. should bubble up here
		
		// first, remove all weak ptrs that are no longer pointing to valid memory
		// @TODO: possibly add hasEdge method to GraphNode
		auto i = srcNode->edges.begin();
		while (i != srcNode->edges.end()) {
			if (auto sptr = (*i)->destNode.lock()) {
				if (equals(sptr->value, dest) && equals((*i)->weight, weight)) {
					return false; // identical edge already exists, return false
				}
				++i; // increment iterator if the weak_ptr was still valid
			} else {
				// weak_ptr invalidated, remove from vector
				srcNode->edges.erase(i);
			}
		}

		// no identical edge exists, we can create one now
		std::shared_ptr<GraphEdge> edge = std::make_shared<GraphEdge>();
		edge->destNode = destNode;
		edge->weight = weight;
		// add the new edge to the srcNode
		srcNode->edges.push_back(edge);
		return true;
	}

	// replaces data stored at the node with the given value
	template <typename N, typename E>
	bool Graph<N, E>::replace(const N& replace, const N& with) {
		auto sptrReplace = getNode(replace);
		// getNode will throw runtime_error if node with value 'replace' isn't found
		if (isNode(with)) {
			return false; // the 'with' value already exists on this graph, return false
		}
		// actually do the replace
		sptrReplace->value = with;
		return true;
	}

	// merges the edges of the two given nodes. first node is destroyed
	template <typename N, typename E>
	void Graph<N, E>::mergeReplace(const N& destroy, const N& second) {
		auto sptrDestroy = getNode(destroy);
		auto sptrSecond = getNode(second);
		// getNode() will throw std::runtime_error if either of the nodes don't exist
		// delete all edges from second to destroy and destroy to second
		sptrSecond->deleteEdgesTo(destroy);
		sptrDestroy->deleteEdgesTo(second);
		// take all of 'destroy's edges and give them to 'second'
		sptrDestroy->giveEdgesTo(sptrSecond);
		// actually get rid of 'destroy' by removing it from Graph::nodes
		// std::vector<std::shared_ptr<GraphNode>>::iterator
		auto pos = std::find(nodes.begin(), nodes.end(), sptrDestroy); // find position of sptrDestroy
		if (pos != nodes.end()) {
			nodes.erase(pos); // actually destroy it
		}
	}

	// deletes node with given value, as well as all edges connected to and from it
	template <typename N, typename E>
	void Graph<N, E>::deleteNode(const N& del) noexcept {
		// @TODO
	}

	// deletes an edge between two nodes with a given weight. doesn't throw exceptions
	template <typename N, typename E>
	void Graph<N, E>::deleteEdge(const N& src, const N& dest, const E& weight) noexcept {
		// @TODO
	}

	// checks if a node with the given value already exists
	template <typename N, typename E>
	bool Graph<N, E>::isNode(const N& n) {
		for (auto i = nodes.begin(); i != nodes.end(); ++i) {
			if (equals((*i)->value, n)) { // iterator 'i' deferences to a smart pointer
				return true; // found
			}
		}
		return false; // not found
	}

	// checks if there is an edge from the first node to the second
	template <typename N, typename E>
	bool Graph<N, E>::isConnected(const N& a, const N& b) {
		auto sptrNodeA = getNode(a);
		auto sptrNodeB = getNode(b);
		// if either a or b don't exist, the getNode() function will throw std::runtime_error
		return sptrNodeA->hasEdgeTo(b);
	}

	// prints out all nodes in this graph
	template <typename N, typename E>
	void Graph<N, E>::printNodes() {
		// sort first
		sortNodes();
		//std::cout << "After sorting:" << std::endl;
		// then print out values
		for (auto i = nodes.begin(); i != nodes.end(); ++i) {
			std::cout << (*i)->value << std::endl;
		}
	}

	// prints all edges of the node with the given value
	// @TODO test not found runtime_error
	template <typename N, typename E>
	void Graph<N, E>::printEdges(const N& n) {
		std::shared_ptr<GraphNode> node = getNode(n); // getNode will throw runtime_error if n not found
		std::cout << "Edges attached to Node " << n << std::endl;
		if (node->edges.size() == 0) { // special case for 0 edges
			std::cout << "(null)" << std::endl;
		}
		// sort all edges on the node
		node->sortEdges();
		// then, print out all edges
		for (auto i = node->edges.begin(); i != node->edges.end(); ++i) {
			if (auto sptr = (*i)->destNode.lock()) {
				std::cout << sptr->value << " " << (*i)->weight << std::endl;
			}
		}
	}
};

#endif
