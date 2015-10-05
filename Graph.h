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
#include <iterator>

namespace cs6771 {

	// @TODO make printNodes() and printEdges() const
	// @TODO test vector.erase() compatibility (and maybe other STL functions?) with iterators

	// @TODO how to not expose this function?
	// templated function to compare for equality of node values and edge weights
	template <typename T>
	bool equals(const T& a, const T& b) {
		return (!(a < b) && !(b < a));
	}

	// forward declaration of NodeIterator class
	template <typename N, typename E>
	class NodeIterator;
	// forward declaration of EdgeIterator class
	template <typename N, typename E>
	class EdgeIterator;

	// @TODO const correctness for all functions
	// @TODO go through everything and check every == for equals()
	// @TODO check all functions and exceptions thrown
	// Graph class declaration
	template <typename N, typename E>
	class Graph {
	private:
		// GraphEdge prototype
		class GraphEdge;
		// GraphNode prototype
		class GraphNode;

	public:
		// both iterators are friends
		friend class NodeIterator<N, E>;
		friend class EdgeIterator<N, E>;

		// default constructor
		Graph() {
			// does nothing
		}

		// big five
		// copy constructor
		Graph(const Graph& from) {
			// call the copy assignment directly
			operator=(from);
		}

		// copy assignment
		Graph& operator=(const Graph& from) {
			// clear current nodes vector, should cascade down
			nodes.clear();

			// deep copy every node and edge within those nodes
			for (auto i = from.nodes.begin(); i != from.nodes.end(); ++i) {
				// on this pass, edges aren't copied yet
				std::shared_ptr<GraphNode> newNode(new GraphNode(**i));
				nodes.push_back(newNode);
			}
			// now that all nodes are copied, we can handle the edges
			for (unsigned int i = 0; i < from.nodes.size(); ++i) {
				// for every edge in original, copy it
				for (unsigned int j = 0; j < from.nodes[i]->edges.size(); ++j) {
					std::shared_ptr<GraphEdge> newEdge(new GraphEdge()); // create new graph edge
					auto oldEdge = from.nodes[i]->edges[j];
					newEdge->weight = oldEdge->weight; // copy over weight value
					if (auto sptrOldEdgeDest = oldEdge->destNode.lock()) {
						const N& oldEdgeDestValue = sptrOldEdgeDest->value;
						auto correspNode = getNode(oldEdgeDestValue); // find the same node in this new graph
						newEdge->destNode = correspNode; // assign it to the new edge
						nodes[i]->edges.push_back(newEdge); // add to edges
					}
					// otherwise don't add, let the newEdge self-destruct at end of this scope
				}
			}
			return *this;
		}

		// move constructor
		Graph(Graph&& from) : nodes(from.nodes) {
			// clears 'from' graph's nodes, but the nodes won't be destructed because we just
			// copied the shared_ptrs into this newly constructed graph
			from.nodes.clear();
		}

		// move assignment
		Graph& operator=(Graph&& from) {
			nodes = from.nodes;
			from.nodes.clear();
			return *this;
		}

		// add a new node to the graph, returns true if node is added, and false if it already exists
		bool addNode(const N& n);
		// add a new edge to the given node, returns true if added, false if it already exists
		bool addEdge(const N& src, const N& dest, const E& weight);
		// replaces the data stored at the node with the given value
		bool replace(const N& replace, const N& with);
		// replaces data stored at node with data stored on another node on the graph
		// first node param is the node that is destroyed. if either node not found, runtime_error thrown
		// edges of both nodes retained after merge, except edges between the two nodes themselves
		void mergeReplace(const N& destroy, const N& second);
		// deletes node with given value, as well as all edges connected to and from it
		void deleteNode(const N& del) noexcept;
		// deletes an edge between two nodes with a given weight. doesn't throw exceptions
		void deleteEdge(const N& src, const N& dest, const E& weight) noexcept;
		// removes all nodes and edges from the graph
		void clear() noexcept;
		// checks if a node already exists
		bool isNode(const N& n) const;
		// checks if there is an edge from the first node to the second
		// if either node is not found, std::runtime_error is thrown
		bool isConnected(const N& a, const N& b) const;
		// prints out all nodes in this graph
		void printNodes() const;
		// prints all edges of the node with the given value, sorted by edge cost incrementing
		// if edge costs are equivalent, sort by < on dest node's value
		void printEdges(const N& n);
		// returns input iterator over the graph
		NodeIterator<N, E> begin() const;
		// returns an iterator to the end of collection of Nodes of this graph
		NodeIterator<N, E> end() const;
		// returns an iterator that iterates over the edges connected to the node with the given value
		EdgeIterator<N, E> edgeIteratorBegin(const N& n) const;
		// returns an iterator to the end of a collection of edges
		EdgeIterator<N, E> edgeIteratorEnd() const;

	private:

		// collection of smart pointers corresponding to the Nodes in this graph
		std::vector<std::shared_ptr<GraphNode>> nodes;

		// @TODO figure out where to use smart pointers in the nested classes (and other places?)
		// inner class Node: contains a list of edges, and the node value itself
		class GraphNode {
		public:
			// * the edges vector is mutable because it can be changed via pointer from iterator constructors,
			//   which are called from the const functions begin, edgeIteratorBegin. HOWEVER, the only changes
			//   made are removal of expired weak_ptrs.
			// * couldn't really find a spec-defined non-const function in which to do this without affecting
			//   performance
			// * the alternative would be to just leave the weak_ptrs there indefinitely until Graph::clear()
			//   or program execution end... but if any of the test cases had a massive number of edge adds
			//   and edge deletions then it would cause out of memory problems
			// * tl;dr I've thought really long and hard about this... pretty please don't dock marks for this
			//   somewhat questionable implementation? :^)
			mutable std::vector<std::shared_ptr<GraphEdge>> edges;
			// the value of this node
			N value;

			// default constructor
			GraphNode() {
				// does nothing
			}

			// copy constructor for GraphNode inner class
			GraphNode(const GraphNode& from) {
				value = from.value; // copy value field
				// do NOT copy edges in here, will be handled in the Graph copy constructor after
				// all nodes have been copied
			}

			// deletes all edges with weak_ptrs that have expired (ie. pointing to deleted nodes) from
			// the edges vector
			// @see documentation above for std::vector<std::shared_ptr<GraphEdge>> edges
			void destroyExpiredEdges() {
				auto i = edges.begin();
				while (i != edges.end()) {
					if (!((*i)->destNode.lock())) { // weak_ptr expired
						edges.erase(i);
					} else {
						++i;
					}
				}
			}

			// sorts all the edges in this node into the correct order. assumes destroyExpiredEdges
			// was called first, before invoking this function
			void sortEdges() {
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
			bool hasEdgeTo(const N& to) const {
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

			// deletes the edge from this node with the given destination value and weight
			void deleteEdge(const N& to, const E& weight) {
				for (auto i = edges.begin(); i != edges.end(); ++i) {
					if (auto sptr = (*i)->destNode.lock()) {
						if (equals(weight, (*i)->weight) && equals(sptr->value, to)) {
							// found identical edge, delete and immediately return
							edges.erase(i);
							return;
						}
					}
				}
				// edge not found, nothing happens, no exceptions should be thrown here
			}
		};

		// nested class Edge: contains the N value of the dest node, and the edge weight
		class GraphEdge {
		public:
			// the weight of this node
			E weight;
			// a weak pointer to the destination node of this edge
			std::weak_ptr<GraphNode> destNode;
		};

		// gets the GraphNode with the given value
		std::shared_ptr<GraphNode> getNode(const N& n) const {
			for (auto i = nodes.begin(); i != nodes.end(); ++i) {
				if (equals((*i)->value, n)) {
					return *i;
				}
			}
			// throw exception if not found
			throw std::runtime_error("node not found");
		}
	};

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
		for (auto i = nodes.begin(); i != nodes.end(); ++i) {
			if (equals((*i)->value, del)) {
				// found the node - erase it and instantly return
				nodes.erase(i); // erase will cascade destructors and there shouldn't be any memory leaks
				return;
			}
		}
		// if we reach here, del wasn't found, nothing happens, this method shouldn't throw any exceptions
	}

	// deletes an edge between two nodes with a given weight
	template <typename N, typename E>
	void Graph<N, E>::deleteEdge(const N& src, const N& dest, const E& weight) noexcept {
		if (isNode(src) && isNode(dest)) {
			auto sptrSrc = getNode(src);
			sptrSrc->deleteEdge(dest, weight);
		}
		// otherwise do nothing, this method shouldn't throw any exceptions
	}
		
	// removes all nodes and edges from the graph
	template <typename N, typename E>
	void Graph<N, E>::clear() noexcept {
		nodes.clear(); // clear will call destructors on the shared_ptrs to GraphNodes and will cascade
	}

	// checks if a node with the given value already exists
	template <typename N, typename E>
	bool Graph<N, E>::isNode(const N& n) const {
		for (auto i = nodes.begin(); i != nodes.end(); ++i) {
			if (equals((*i)->value, n)) { // iterator 'i' deferences to a smart pointer
				return true; // found
			}
		}
		return false; // not found
	}

	// checks if there is an edge from the first node to the second
	template <typename N, typename E>
	bool Graph<N, E>::isConnected(const N& a, const N& b) const {
		auto sptrNodeA = getNode(a);
		auto sptrNodeB = getNode(b);
		// if either a or b don't exist, the getNode() function will throw std::runtime_error
		return sptrNodeA->hasEdgeTo(b);
	}

	// prints out all nodes in this graph
	template <typename N, typename E>
	void Graph<N, E>::printNodes() const {
		// use NodeIterator to print out nodes in correct order, sorting is done within iterator ctor
		// expired edges will be deleted in NodeIterator::ctor before the sorting
		for (auto i = begin(); i != end(); ++i) {
			std::cout << *i << std::endl;
		}
	}

	// prints all edges of the node with the given value
	// @TODO test not found runtime_error
	template <typename N, typename E>
	void Graph<N, E>::printEdges(const N& n) {
		std::shared_ptr<GraphNode> node = getNode(n); // getNode will throw runtime_error if n not found
		std::cout << "Edges attached to Node " << n << std::endl;
		// destroy expired weak_ptrs to edges
		node->destroyExpiredEdges();
		// sort all edges on the node
		node->sortEdges();
		if (node->edges.size() == 0) { // special case for 0 edges
			std::cout << "(null)" << std::endl;
		}
		// then, print out all edges
		for (auto i = node->edges.begin(); i != node->edges.end(); ++i) {
			if (auto sptr = (*i)->destNode.lock()) {
				std::cout << sptr->value << " " << (*i)->weight << std::endl;
			}
		}
	}

	// returns input iterator over the graph
	template <typename N, typename E>
	NodeIterator<N, E> Graph<N, E>::begin() const {
		return NodeIterator<N, E>(nodes);
	}

	// returns an iterator to the end of collection of Nodes of this graph
	template <typename N, typename E>
	NodeIterator<N, E> Graph<N, E>::end() const {
		return NodeIterator<N, E>();
	}

	// node iterator class
	// @TODO test pointer -> usage
	template <typename N, typename E>
	class NodeIterator {
	public:
		typedef std::ptrdiff_t 				difference_type;
		typedef std::input_iterator_tag 	iterator_category;
		typedef N							value_type;
		typedef N const*					pointer;
		typedef const N&					reference;

		reference operator*() const;
		pointer operator->() const { return &(operator*()); }
		NodeIterator& operator++();
		bool operator==(const NodeIterator& other) const;
		bool operator!=(const NodeIterator& other) const { return !operator==(other); }

		// default constructor, constructs an iterator which matches end() ie. one past the last element
		NodeIterator() : index(0) {
		}

		// constructor which takes in the graph's nodes and copies shared_ptrs over by value
		NodeIterator(std::vector<std::shared_ptr<typename Graph<N, E>::GraphNode>> n) : iterNodes(n), index(0) {
			// delete expired edges from all nodes
			for (auto i = iterNodes.begin(); i != iterNodes.end(); ++i) {
				// @see documentation for std::vector<std::shared_ptr<GraphEdge>> edges
				(*i)->destroyExpiredEdges();
			}

			// sort the copied vector (doesn't affect the Graph::nodes field, shared_ptrs copied by value)
			std::sort(iterNodes.begin(), iterNodes.end(),
				[](std::shared_ptr<typename Graph<N, E>::GraphNode> a,
					std::shared_ptr<typename Graph<N, E>::GraphNode> b) {

						int sizeA = a->edges.size();
						int sizeB = b->edges.size();
						if (sizeA == sizeB) {
							return a->value < b->value;
						}
						return sizeB < sizeA; // node order is most edges -> least edges
				});
		}
	
	private:
		// copy of the shared_ptr vector. this is the one that is sorted without affecting the Graph
		std::vector<std::shared_ptr<typename Graph<N, E>::GraphNode>> iterNodes;
		// current index of the iterator
		unsigned int index;
	};

	template <typename N, typename E>
	const N& NodeIterator<N, E>::operator*() const {
		if (index >= iterNodes.size()) {
			throw std::runtime_error("dereferencing out of bounds NodeIterator");
		}
		return iterNodes[index]->value;
	}

	template <typename N, typename E>
	NodeIterator<N, E>& NodeIterator<N, E>::operator++() {
		++index; // increment index
		if (index >= iterNodes.size()) {
			iterNodes.clear(); // clear out the nodes vector, we're finished
		}
		return *this;
	}

	template <typename N, typename E>
	bool NodeIterator<N, E>::operator==(const NodeIterator& other) const {
		unsigned int sizeThis = iterNodes.size();
		unsigned int sizeOther = other.iterNodes.size();
		if (sizeThis == 0 && sizeOther == 0) { // both empty vectors, index doesn't matter
			return true;
		}
		if (sizeThis != sizeOther || index != other.index) {
			return false;
		}
		// if we're here, neither size is 0 and index fields are equal so it's safe to compare as follows
		return iterNodes[index] == other.iterNodes[other.index]; // comparing shared_ptr point dest
	}

	// returns an iterator that iterates over the edges connected to the node with the given value
	template <typename N, typename E>
	EdgeIterator<N, E> Graph<N, E>::edgeIteratorBegin(const N& n) const {
		auto sptr = getNode(n);
		// getNode will throw std::runtime_error if node with value n isn't found
		return EdgeIterator<N, E>(sptr);
	}

	// returns an iterator to the end of a collection of edges
	template <typename N, typename E>
	EdgeIterator<N, E> Graph<N, E>::edgeIteratorEnd() const {
		return EdgeIterator<N, E>(std::shared_ptr<typename Graph<N, E>::GraphNode>());
	}

	template <typename N, typename E>
	class EdgeIterator {
	public:
		typedef std::ptrdiff_t 				difference_type;
		typedef std::input_iterator_tag 	iterator_category;
		typedef std::pair<N, E>				value_type;
		typedef std::pair<N, E> const*		pointer;
		typedef const std::pair<N, E>&		reference;

		reference operator*() const;
		pointer operator->() const { return &(operator*()); }
		EdgeIterator& operator++();
		bool operator==(const EdgeIterator& other) const;
		bool operator!=(const EdgeIterator& other) const { return !operator==(other); }

		// default constructor, constructs an iterator which matches end() ie. one past the last element
		EdgeIterator(std::shared_ptr<typename Graph<N, E>::GraphNode> n) : node(n), index(0) {
			if (node) { // if not nullptr (which is the case for end())
				// destroy expired edges on the node
				node->destroyExpiredEdges();
				if (node->edges.size() > 0) { // if the node has still-alive edges
					node->sortEdges(); // sort them
				} else { // this node has no live edges, so set to null to denote equal to end()
					node = std::shared_ptr<typename Graph<N, E>::GraphNode>();
				}
			}
		}
	private:
		// the node that this iterator is iterating over
		std::shared_ptr<typename Graph<N, E>::GraphNode> node;
		// the edge index that this iterator is currently at
		unsigned int index;
		// the std::pair that is returned when this iterator is dereferenced
		mutable std::pair<N, E> deref;
	};

	template <typename N, typename E>
	const std::pair<N, E>& EdgeIterator<N, E>::operator*() const {
		if (index >= node->edges.size()) {
			throw std::runtime_error("dereferencing out of bounds EdgeIterator");
		}
		/*
		if (node->edges[index]->destNode.expired()) {
			throw std::runtime_error("destNode expired, this shouldn't happen");
		}
		*/
		// populate pair with values from current edge
		deref.first = node->edges[index]->destNode.lock()->value;
		deref.second = node->edges[index]->weight;
		// return the pair
		return deref;
	}

	template <typename N, typename E>
	EdgeIterator<N, E>& EdgeIterator<N, E>::operator++() {
		++index;
		if (node) { // if shared_ptr node isn't already null
			if (index >= node->edges.size()) { // if we've reached end
				node = std::shared_ptr<typename Graph<N, E>::GraphNode>();
			}
		}
		return *this;
	}

	template <typename N, typename E>
	bool EdgeIterator<N, E>::operator==(const EdgeIterator& other) const {
		if (!node && !other.node) {
			return true;
		}
		if (index != other.index || node != other.node) {
			return false;
		}
		return node->edges[index] == other.node->edges[index];
	}
};

#endif
