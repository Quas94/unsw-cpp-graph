/**
 * COMP6771 Assignment 3
 * Michael Su - z3466178
 *
 * Submission for ass3 - a generic directed value-like graph container library
 */
#ifndef GRAPH_H_GUARD
#define GRAPH_H_GUARD

#include <iostream>
#include <algorithm>
#include <vector>


namespace cs6771 {

	// Graph class declaration
	template <typename N, typename E>
	class Graph {
	public:
		// default constructor
		Graph();
		// add a new node to the graph, returns true if node is added, and false if it already exists
		bool addNode(const N& node);
		// checks if a node already exists
		bool isNode(const N& node);
		// prints out all nodes in this graph
		void printNodes();

	private:
		std::vector<N> nodes;
		std::vector<E> edges;

		// sort Nodes into correct order
		// @TODO: implement mutable sort flag to prevent unnecessary sorting
		void sortNodes();
	};

	// default constructor definition
	template <typename N, typename E>
	Graph<N, E>::Graph() {
		// do nothing as of now
	}

	// add a new node to the graph
	template <typename N, typename E>
	bool Graph<N, E>::addNode(const N& node) {
		if (!isNode(node)) { // doesn't exist, add it in
			nodes.push_back(node);
			return true;
		}
		return false; // already exists so false
	}

	// checks if a node already exists
	template <typename N, typename E>
	bool Graph<N, E>::isNode(const N& node) {
		return (std::find(nodes.begin(), nodes.end(), node) != nodes.end());
	}

	// prints out all nodes in this graph
	template <typename N, typename E>
	void Graph<N, E>::printNodes() {
		// sort first
		sortNodes();
		// then print out
		for (auto i = nodes.begin(); i != nodes.end(); ++i) {
			std::cout << *i << std::endl;
		}
	}

	// sort comparator function for std::sort on graph nodes
	// @TODO take into account number of edges, currently only sorts with <
	// @TODO turn into lambda function
	/*
	template <typename N>
	bool sortNodesComparator(const N& a, const N& b) {
		return (a < b);
	}
	*/

	// sorts nodes into correct order (number of edges descending order, then < on node value)
	// @TODO sort properly using sortNodesComparator
	template <typename N, typename E>
	void Graph<N, E>::sortNodes() {
		std::sort(nodes.begin(), nodes.end());
	}
};

#endif
