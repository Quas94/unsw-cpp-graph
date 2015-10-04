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
		// checks if a node already exists
		bool isNode(const N& n);
		// prints out all nodes in this graph
		void printNodes();

		// add a new edge to the given node, returns true if added, false if it already exists
		bool addEdge(const N& src, const N& dest, const E& weight);
		// prints all edges of the node with the given value, sorted by edge cost incrementing
		// if edge costs are equivalent, sort by < on dest node's value
		void printEdges(const N& n);

		// checks if there is an edge from the first node to the second
		// if either node is not found, std::runtime_error is thrown
		bool isConnected(const N& a, const N& b);

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

	// checks if there is an edge from the first node to the second
	template <typename N, typename E>
	bool Graph<N, E>::isConnected(const N& a, const N& b) {
		auto sptrNodeA = getNode(a);
		auto sptrNodeB = getNode(b);
		// if either a or b don't exist, the getNode() function will throw std::runtime_error
		return sptrNodeA->hasEdgeTo(b);
	}
};

#endif
