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
		void sortNodes();

		// gets the GraphNode with the given value
		std::shared_ptr<GraphNode> getNode(const N& n) {
			for (auto i = nodes.begin(); i != nodes.end(); ++i) {
				if ((*i)->value == n) {
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
			if ((*i)->value == n) { // iterator 'i' deferences to a smart pointer
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
				if (sptr->value == dest && (*i)->weight == weight) {
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
		// @TODO: sort by increasing edge cost, if edge costs equal, sort by < on dest node value
		// then, print out all edges
		for (auto i = node->edges.begin(); i != node->edges.end(); ++i) {
			if (auto sptr = (*i)->destNode.lock()) {
				std::cout << sptr->value << " " << (*i)->weight << std::endl;
			}
		}
	}

	// sort comparator function for std::sort on graph nodes
	// @TODO take into account number of edges, currently only sorts with <
	// @TODO turn into lambda function?
	/*
	bool sortNodesComparator(const Graph<N, E>::GraphNode a, const Graph<N, E>::GraphNode b) {
		return (a < b);
	}
	*/

	// sorts nodes into correct order (number of edges descending order, then < on node value)
	// @TODO sort properly using sortNodesComparator
	template <typename N, typename E>
	void Graph<N, E>::sortNodes() {
		//std::sort(nodes.begin(), nodes.end(), sortNodesComparator);
	}
};

#endif
