#ifndef OMNI_UTILS_HPP_
#define OMNI_UTILS_HPP_

#include "queue_iterator.hpp"
#include "logging.hpp"
#include "simple_tools.hpp"

namespace omnigraph {

LOGGER("omg.graph");

/**
 * GraphActionHandler is base listening class for graph events. All structures and information storages
 * which are meant to synchronize with graph should use this structure. In order to make handler listen
 * to graph events one should add it to graph listeners.
 * Normally structure itself extends GraphActionHandler and overrides several handling methods. In
 * constructor it adds itself to graph handler list and removes itself form this list in destructor.
 * All events are divided into two levels: low level events and high level events.
 * Low level events are addition/deletion of vertices/edges. These events should be triggered only after
 * high level events when all data was already transferred and graph structure is consistent.
 * High level events should be used to keep external data synchronized with graph and keep internal data
 * consistent. Now high level events are merge, glue and split. This list can be extended in near future.
 */
template<class Graph>
class GraphActionHandler {
public:
	typedef typename Graph::VertexId VertexId;
	typedef typename Graph::EdgeId EdgeId;

	const string handler_name_;

	/**
	 * Create action handler with given name. With this name one can find out what tipe of handler is it.
	 */
	GraphActionHandler(const string &name) :
		handler_name_(name) {
	}

	/**
	 * Method returns name of this handler
	 */
	const string &name() const {
		return handler_name_;
	}

	/**
	 * Low level event which is triggered when vertex is added to graph.
	 * @param v new vertex
	 */
	virtual void HandleAdd(VertexId v) {
	}

	/**
	 * Low level event which is triggered when edge is added to graph.
	 * @param e new edge
	 */
	virtual void HandleAdd(EdgeId e) {
	}

	/**
	 * Low level event which is triggered when vertex is deleted from graph.
	 * @param v vertex to delete
	 */
	virtual void HandleDelete(VertexId v) {
	}

	/**
	 * Low level event which is triggered when edge is deleted from graph.
	 * @param e edge to delete
	 */
	virtual void HandleDelete(EdgeId e) {
	}

	/**
	 * High level event which is triggered when merge operation is performed on graph, which is when
	 * path of edges with all inner vertices having exactly one incoming and one outgoing edge is
	 * replaced with a single edge. Since this is high level operation event of creation of new edge
	 * and events of deletion of old edges should not have been triggered yet when this event was triggered.
	 * @param old_edges path of edges to be replaced with single edge
	 * @param new_edge new edge that was added to be a replacement of path
	 */
	virtual void HandleMerge(vector<EdgeId> old_edges, EdgeId new_edge) {
	}

	/**
	 * High level event which is triggered when glue operation is performed on graph, which is when
	 * edge is completely replaced with other edge. This operation is widely used in bulge removal
	 * when alternative path is glued to main path. Since this is high level operation event of deletion
	 * of old edge should not have been triggered yet when this event was triggered.
	 * @param old_edge edge to be glued to new edge
	 * @param new_edge edge old edge should be glued to
	 */
	virtual void HandleGlue(EdgeId old_edge, EdgeId new_edge) {
	}

	/**
	 * High level event which is triggered when split operation is performed on graph, which is when
	 * edge is split into several shorter edges. Split operation is reverse to merge operation.
	 * Since this is high level operation event of deletion of old edge and events of creation of new edges
	 * should not have been triggered yet when this event was triggered.
	 * @param old_edge edge to be split
	 * @param new_edges edges which are results of split
	 */
	virtual void HandleSplit(EdgeId old_edge, EdgeId new_edge_1,
			EdgeId new_edge2) {
	}

	virtual ~GraphActionHandler() {
	}
};

/**
 * In order to support various types of graphs and make handler structure more flexible HandlerApplier
 * structure was introduced. If certain implementation of graph requires special handler triggering scheme
 * one can store certain extension of HandlerApplier in graph and trigger HandlerApplier methods instead
 * of GraphHandler methods.
 * HandlerApplier contains one method for each of graph events which define the exact way this event
 * should be triggered.
 */
template<class Graph>
class HandlerApplier {
public:
	typedef typename Graph::VertexId VertexId;
	typedef typename Graph::EdgeId EdgeId;

	virtual void
	ApplyAdd(GraphActionHandler<Graph> *handler, VertexId v) const = 0;

	virtual void
	ApplyAdd(GraphActionHandler<Graph> *handler, EdgeId e) const = 0;

	virtual void
	ApplyDelete(GraphActionHandler<Graph> *handler, VertexId v) const = 0;

	virtual void
	ApplyDelete(GraphActionHandler<Graph> *handler, EdgeId e) const = 0;

	virtual void ApplyMerge(GraphActionHandler<Graph> *handler,
			vector<EdgeId> old_edges, EdgeId new_edge) const = 0;

	virtual void ApplyGlue(GraphActionHandler<Graph> *handler, EdgeId old_edge,
			EdgeId new_edge) const = 0;

	virtual void ApplySplit(GraphActionHandler<Graph> *handler,
			EdgeId old_edge, EdgeId new_edge_1, EdgeId new_edge2) const = 0;

	virtual ~HandlerApplier() {
	}
};

/**
 * SimpleHandlerApplier is simple implementation of handler applier with no special filtering.
 */
template<class Graph>
class SimpleHandlerApplier: public HandlerApplier<Graph> {
public:
	typedef typename Graph::VertexId VertexId;
	typedef typename Graph::EdgeId EdgeId;

	virtual void ApplyAdd(GraphActionHandler<Graph> *handler, VertexId v) const {
		handler.HandleAdd(v);
	}

	virtual void ApplyAdd(GraphActionHandler<Graph> *handler, EdgeId e) const {
		handler.HandleAdd(e);
	}

	virtual void ApplyDelete(GraphActionHandler<Graph> *handler, VertexId v) const {
		handler.HandleDelete(v);
	}

	virtual void ApplyDelete(GraphActionHandler<Graph> *handler, EdgeId e) const {
		handler.HandleDelete(e);
	}

	virtual void ApplyMerge(GraphActionHandler<Graph> *handler,
			vector<EdgeId> old_edges, EdgeId new_edge) const {
		handler.HandleMerge(old_edges, new_edge);
	}

	virtual void ApplyGlue(GraphActionHandler<Graph> *handler, EdgeId old_edge,
			EdgeId new_edge) const {
		handler.HandleGlue(old_edge, new_edge);
	}

	virtual void ApplySplit(GraphActionHandler<Graph> *handler,
			EdgeId old_edge, EdgeId new_edge1, EdgeId new_edge2) const {
		handler.HandleSplit(old_edge, new_edge1, new_edge2);
	}

	virtual ~SimpleHandlerApplier() {
	}
};

/**
 * PairedHandlerApplier is implementation of HandlerApplier for graph with synchronization of actions
 * performed with vertices/edges and its reverse-complement analogues. Thus while corresponding
 * method was called only once event should be triggered twice: for the parameters with which method
 * was called and for reverse-complement parameters. Also certain assertions were added for bad cases.
 */
template<class Graph>
class PairedHandlerApplier: public HandlerApplier<Graph> {
private:
	Graph &graph_;
public:
	typedef typename Graph::VertexId VertexId;
	typedef typename Graph::EdgeId EdgeId;

	PairedHandlerApplier(Graph &graph) :
		graph_(graph) {
	}

	virtual void ApplyAdd(GraphActionHandler<Graph> *handler, VertexId v) const {
		VertexId rcv = graph_.conjugate(v);
		TRACE(
				"Triggering add event of handler " << handler->name()
						<< " to vertex " << v);
		handler->HandleAdd(v);
		if (v != rcv) {
			TRACE(
					"Triggering add event of handler " << handler->name()
							<< " to vertex " << rcv
							<< " which is conjugate to " << v);
			handler->HandleAdd(rcv);
		} else {
			TRACE(
					"Vertex " << v
							<< "is self-conjugate thus handler is not applied the second time");
		}
	}

	virtual void ApplyAdd(GraphActionHandler<Graph> *handler, EdgeId e) const {
		EdgeId rce = graph_.conjugate(e);
		TRACE(
				"Triggering add event of handler " << handler->name()
						<< " to edge " << e << ". Event is Add");
		handler->HandleAdd(e);
		if (e != rce) {
			TRACE(
					"Triggering add event of handler " << handler->name()
							<< " to edge " << rce << " which is conjugate to "
							<< e);
			handler->HandleAdd(rce);
		} else {
			TRACE(
					"Edge " << e
							<< "is self-conjugate thus handler is not applied the second time");
		}
	}

	virtual void ApplyDelete(GraphActionHandler<Graph> *handler, VertexId v) const {
		VertexId rcv = graph_.conjugate(v);
		TRACE(
				"Triggering delete event of handler " << handler->name()
						<< " to vertex " << v);
		handler->HandleDelete(v);
		if (v != rcv) {
			TRACE(
					"Triggering delete event of handler " << handler->name()
							<< " to vertex " << rcv
							<< " which is conjugate to " << v);
			handler->HandleDelete(rcv);
		} else {
			TRACE(
					"Vertex " << v
							<< "is self-conjugate thus handler is not applied the second time");
		}
	}

	virtual void ApplyDelete(GraphActionHandler<Graph> *handler, EdgeId e) const {
		EdgeId rce = graph_.conjugate(e);
		TRACE(
				"Triggering delete event of handler " << handler->name()
						<< " to edge " << e);
		handler->HandleDelete(e);
		if (e != rce) {
			TRACE(
					"Triggering delete event of handler " << handler->name()
							<< " to edge " << rce << " which is conjugate to "
							<< e);
			handler->HandleDelete(rce);
		} else {
			TRACE(
					"Edge " << e
							<< "is self-conjugate thus handler is not applied the second time");
		}

	}

	virtual void ApplyMerge(GraphActionHandler<Graph> *handler,
			vector<EdgeId> old_edges, EdgeId new_edge) const {
		TRACE(
				"Triggering merge event of handler " << handler->name()
						<< " with new edge " << new_edge);
		EdgeId rce = graph_.conjugate(new_edge);
		handler->HandleMerge(old_edges, new_edge);
		if (new_edge != rce) {
			TRACE(
					"Triggering merge event of handler " << handler->name()
							<< " with new edge " << rce
							<< " which is conjugate to " << new_edge);
			vector < EdgeId > ecOldEdges;
			for (int i = old_edges.size() - 1; i >= 0; i--) {
				ecOldEdges.push_back(graph_.conjugate(old_edges[i]));
			}
			handler->HandleMerge(ecOldEdges, rce);
		} else {
			TRACE(
					"Edge " << new_edge
							<< "is self-conjugate thus handler is not applied the second time");
		}
	}

	virtual void ApplyGlue(GraphActionHandler<Graph> *handler, EdgeId old_edge,
			EdgeId new_edge) const {
		TRACE(
				"Triggering glue event of handler " << handler->name()
						<< " with old edge " << old_edge);
		EdgeId rcOldEdge = graph_.conjugate(old_edge);
		EdgeId rcNewEdge = graph_.conjugate(new_edge);
		assert(old_edge != new_edge);
		assert(new_edge != rcNewEdge);
		assert(graph_.EdgeStart(old_edge) != graph_.EdgeEnd(old_edge));
		assert(graph_.EdgeStart(new_edge) != graph_.EdgeEnd(new_edge));
		handler->HandleGlue(old_edge, new_edge);
		if (old_edge != rcOldEdge) {
			TRACE(
					"Triggering merge event of handler " << handler->name()
							<< " with old edge " << old_edge
							<< " which is conjugate to " << rcOldEdge);
			handler->HandleGlue(rcOldEdge, rcNewEdge);
		} else {
			TRACE(
					"Edge " << old_edge
							<< "is self-conjugate thus handler is not applied the second time");
		}
	}

	virtual void ApplySplit(GraphActionHandler<Graph> *handler,
			EdgeId old_edge, EdgeId new_edge_1, EdgeId new_edge2) const {
		EdgeId rce = graph_.conjugate(old_edge);
		TRACE(
				"Triggering split event of handler " << handler->name()
						<< " with old edge " << old_edge);
		handler->HandleSplit(old_edge, new_edge_1, new_edge2);
		if (old_edge != rce) {
			TRACE(
					"Triggering split event of handler " << handler->name()
							<< " with old edge " << old_edge
							<< " which is conjugate to " << rce);
			handler->HandleSplit(rce, graph_.conjugate(new_edge2),
					graph_.conjugate(new_edge_1));
		} else {
			TRACE(
					"Edge " << old_edge
							<< "is self-conjugate thus handler is not applied the second time");
		}
	}

	virtual ~PairedHandlerApplier() {
	}
};

/**
 * SmartIterator is abstract class which acts both as QueueIterator and GraphActionHandler. As QueueIterator
 * SmartIterator is able to iterate through collection content of which can be changed in process of
 * iteration. And as GraphActionHandler SmartIterator can change collection contents with respect to the
 * way graph is changed. Also one can define order of iteration by specifying Comparator.
 */
template<class Graph, typename ElementId, typename Comparator = std::less<
		ElementId> >
class SmartIterator: public GraphActionHandler<Graph> , public QueueIterator<
		ElementId, Comparator> {
private:
	Graph &graph_;
public:
	typedef QueueIterator<ElementId, Comparator> super;
	typedef typename Graph::VertexId VertexId;
	typedef typename Graph::EdgeId EdgeId;
public:
	SmartIterator(Graph &graph, const string &name,
			const Comparator& comparator = Comparator()) :
		GraphActionHandler<Graph> (name),
				QueueIterator<ElementId, Comparator> (comparator),
				graph_(graph) {
		graph_.AddActionHandler(this);
	}

	virtual ~SmartIterator() {
		graph_.RemoveActionHandler(this);
	}
};

/**
 * SmartVertexIterator iterates through vertices of graph. It listens to AddVertex/DeleteVertex graph events
 * and correspondingly edits the set of vertices to iterate through. Note: high level event handlers are
 * triggered before low level event handlers like HandleAdd/HandleDelete. Thus if Comparator uses certain
 * structure which is also updated with handlers make sure that all information is updated in high level
 * event handlers.
 */
template<class Graph, typename Comparator = std::less<typename Graph::VertexId> >
class SmartVertexIterator: public SmartIterator<Graph,
		typename Graph::VertexId, Comparator> {
public:
	typedef QueueIterator<typename Graph::VertexId, Comparator> super;
	typedef typename Graph::VertexId VertexId;
	typedef typename Graph::EdgeId EdgeId;
public:
	SmartVertexIterator(Graph &graph, bool fill,
			const Comparator& comparator = Comparator()) :
				SmartIterator<Graph, VertexId, Comparator> (graph,
						"SmartVertexIterator " + ToString(this), comparator) {
		if (fill) {
			super::insert(graph.begin(), graph.end());
		}
	}

	virtual ~SmartVertexIterator() {
	}

	virtual void HandleAdd(VertexId v) {
		super::push(v);
	}

	virtual void HandleDelete(VertexId v) {
		super::erase(v);
	}
};

/**
 * SmartEdgeIterator iterates through edges of graph. It listens to AddEdge/DeleteEdge graph events
 * and correspondingly edits the set of edges to iterate through. Note: high level event handlers are
 * triggered before low level event handlers like HandleAdd/HandleDelete. Thus if Comparator uses certain
 * structure which is also updated with handlers make sure that all information is updated in high level
 * event handlers.
 */
template<class Graph, typename Comparator = std::less<typename Graph::EdgeId> >
class SmartEdgeIterator: public SmartIterator<Graph, typename Graph::EdgeId,
		Comparator> {
public:
	typedef QueueIterator<typename Graph::EdgeId, Comparator> super;
	typedef typename Graph::VertexId VertexId;
	typedef typename Graph::EdgeId EdgeId;
public:
	SmartEdgeIterator(Graph &graph, bool fill,
			Comparator comparator = Comparator()) :
				SmartIterator<Graph, EdgeId, Comparator> (graph,
						"SmartEdgeIterator " + ToString(this), comparator) {
		if (fill) {
			for (typename Graph::VertexIterator it = graph.begin(); it
					!= graph.end(); ++it) {
				const vector<EdgeId> outgoing = graph.OutgoingEdges(*it);
				this->super::insert(outgoing.begin(), outgoing.end());
			}
		}
	}

	virtual ~SmartEdgeIterator() {
	}

	virtual void HandleAdd(EdgeId v) {
		super::push(v);
	}

	virtual void HandleDelete(EdgeId v) {
		super::erase(v);
	}
};

}
#endif /* OMNI_UTILS_HPP_ */
