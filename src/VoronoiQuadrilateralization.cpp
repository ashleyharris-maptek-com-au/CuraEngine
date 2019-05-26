//Copyright (c) 2019 Ultimaker B.V.
#include "VoronoiQuadrilateralization.h"


#include "utils/VoronoiUtils.h"

#include "utils/linearAlg2D.h"
#include "utils/IntPoint.h"

#include "utils/logoutput.h"


namespace boost {
namespace polygon {

template <>
struct geometry_concept<arachne::Point>
{
    typedef point_concept type;
};

template <>
struct point_traits<arachne::Point>
{
    typedef int coordinate_type;

    static inline coordinate_type get(
            const arachne::Point& point, orientation_2d orient)
    {
        return (orient == HORIZONTAL) ? point.X : point.Y;
    }
};

template <>
struct geometry_concept<arachne::VoronoiQuadrilateralization::Segment>
{
    typedef segment_concept type;
};

template <>
struct segment_traits<arachne::VoronoiQuadrilateralization::Segment>
{
    typedef arachne::coord_t coordinate_type;
    typedef arachne::Point point_type;
    static inline point_type get(const arachne::VoronoiQuadrilateralization::Segment& segment, direction_1d dir) {
        return dir.to_int() ? segment.p() : segment.next().p();
    }
};
}    // polygon
}    // boost



namespace arachne
{

VoronoiQuadrilateralization::node_t& VoronoiQuadrilateralization::make_node(vd_t::vertex_type& vd_node, Point p)
{
    auto he_node_it = vd_node_to_he_node.find(&vd_node);
    if (he_node_it == vd_node_to_he_node.end())
    {
        graph.nodes.emplace_front(VoronoiQuadrilateralizationJoint(), p);
        node_t& node = graph.nodes.front();
        vd_node_to_he_node.emplace(&vd_node, &node);
        return node;
    }
    else
    {
        return *he_node_it->second;
    }
}

VoronoiQuadrilateralization::edge_t& VoronoiQuadrilateralization::make_edge(Point from, Point to, vd_t::edge_type& vd_edge)
{
    if (vd_edge.cell()->contains_point() || vd_edge.twin()->cell()->contains_point())
    {
        logError("Discretizing segment not implemented yet.\n");
    }
    
    graph.edges.emplace_front(VoronoiQuadrilateralizationEdge());
    edge_t& edge = graph.edges.front();
    vd_edge_to_he_edge.emplace(&vd_edge, &edge);
    
    edge.from = &make_node(*vd_edge.vertex0(), from);
    edge.to = &make_node(*vd_edge.vertex1(), to);
    edge.from->some_edge = &edge;
    edge.to->some_edge = &edge;
    
    auto he_edge_it = vd_edge_to_he_edge.find(vd_edge.twin());
    if (he_edge_it != vd_edge_to_he_edge.end())
    {
        edge.twin = he_edge_it->second;
        he_edge_it->second->twin = &edge;
    }
    
    return edge;
}

VoronoiQuadrilateralization::VoronoiQuadrilateralization(const Polygons& polys)
{
    std::vector<Point> points; // remains empty

    std::vector<Segment> segments;
    for (size_t poly_idx = 0; poly_idx < polys.size(); poly_idx++)
    {
        ConstPolygonRef poly = polys[poly_idx];
        for (size_t point_idx = 0; point_idx < poly.size(); point_idx++)
        {
            segments.emplace_back(&polys, poly_idx, point_idx);
        }
    }

    vd_t vd;
    construct_voronoi(points.begin(), points.end(),
                                        segments.begin(), segments.end(),
                                        &vd);

    VoronoiUtils::debugOutput("output/vd.svg", vd, points, segments);
    
    
    
    for (vd_t::cell_type cell : vd.cells())
    {
        Point start_source_point;
        Point end_source_point;
        vd_t::edge_type* starting_vd_edge = nullptr;
        vd_t::edge_type* ending_vd_edge = nullptr;
        
        if (cell.contains_segment())
        {
            const Segment& source_segment = VoronoiUtils::getSourceSegment(cell, points, segments);
//             printf("source segment: (%lld, %lld) - (%lld, %lld)\n", source_segment.from().X, source_segment.from().Y, source_segment.to().X, source_segment.to().Y);
            // find starting edge
            // find end edge
            bool first = true;
            for (vd_t::edge_type* edge = cell.incident_edge(); edge != cell.incident_edge() || first; edge = edge->next())
            {
                if (edge->is_infinite())
                {
                    first = false;
                    continue;
                }
//                 printf("edge: (%f, %f) - (%f, %f)\n", edge->vertex0()->x(), edge->vertex0()->y(), edge->vertex1()->x(), edge->vertex1()->y());
                if (false && edge->is_secondary())
                { // edge crosses source segment
                    // TODO: handle the case where two consecutive line segments are collinear!
                    // that's the only case where a voronoi segment doesn't end in a polygon vertex, but goes though it
                    if (LinearAlg2D::pointLiesOnTheRightOfLine(VoronoiUtils::p(edge->vertex1()), source_segment.from(), source_segment.to()))
                    {
                        ending_vd_edge = edge;
                    }
                    else
                    {
                        starting_vd_edge = edge;
                    }
                    first = false;
                    continue;
                }
                if (VoronoiUtils::p(edge->vertex0()) == source_segment.to())
                {
                    starting_vd_edge = edge;
                }
                if (VoronoiUtils::p(edge->vertex1()) == source_segment.from())
                {
                    ending_vd_edge = edge;
                }
                first = false;
            }
            
            assert(starting_vd_edge && ending_vd_edge);
            assert(starting_vd_edge != ending_vd_edge);
            
            start_source_point = source_segment.to();
            end_source_point = source_segment.from();
        }
        else
        {
            if (cell.incident_edge()->is_infinite())
            {
                continue;
            }
            // check if any point of the cell is inside or outside polygon
            // copy whole cell into graph or not at all
            
            const Point source_point = VoronoiUtils::getSourcePoint(cell, points, segments);
            const PolygonsPointIndex source_point_index = VoronoiUtils::getSourcePointIndex(cell, points, segments);
            Point some_point = VoronoiUtils::p(cell.incident_edge()->vertex0());
            if (some_point == source_point)
            {
                some_point = VoronoiUtils::p(cell.incident_edge()->vertex1());
            }
            if (!LinearAlg2D::isInsideCorner(source_point_index.prev().p(), source_point_index.p(), source_point_index.next().p(), some_point))
            { // cell is outside of polygon
                continue; // don't copy any part of this cell
            }
            bool first = true;
            for (vd_t::edge_type* vd_edge = cell.incident_edge(); vd_edge != starting_vd_edge || first; vd_edge = vd_edge->next())
            {
                assert(vd_edge->is_finite());
                Point p1 = VoronoiUtils::p(vd_edge->vertex1());
                if (shorterThen(p1 - source_point, snap_dist))
                {
                    start_source_point = source_point;
                    end_source_point = source_point;
                    starting_vd_edge = vd_edge->next();
                    ending_vd_edge = vd_edge;
                }
                first = false;
            }
            assert(starting_vd_edge && ending_vd_edge);
            assert(starting_vd_edge != ending_vd_edge);
        }
        
        // copy start to end edge to graph
        
        
        edge_t* starting_edge = &make_edge(start_source_point, VoronoiUtils::p(starting_vd_edge->vertex1()), *starting_vd_edge);
        // starting_edge->prev = nullptr;
        starting_edge->from->data.distance_to_boundary = 0;
        
        edge_t* prev_edge = starting_edge;
        for (vd_t::edge_type* vd_edge = starting_vd_edge->next(); vd_edge != ending_vd_edge; vd_edge = vd_edge->next())
        {
            assert(vd_edge->is_finite());
            edge_t* edge = &make_edge(VoronoiUtils::p(vd_edge->vertex0()), VoronoiUtils::p(vd_edge->vertex1()), *vd_edge);
            edge->prev = prev_edge;
            prev_edge->next = edge;
            prev_edge = edge;
        }
        
        edge_t* ending_edge = &make_edge(VoronoiUtils::p(ending_vd_edge->vertex0()), end_source_point, *ending_vd_edge);
        ending_edge->prev = prev_edge;
        prev_edge->next = ending_edge;
        // ending_edge->next = nullptr;
        ending_edge->to->data.distance_to_boundary = 0;
        
        
    }
    
    debugCheckGraphCompleteness();
    
    {
        AABB aabb(polys);
        SVG svg("output/graph.svg", aabb);
        graph.debugOutput(svg);
        svg.writePolygons(polys, SVG::Color::BLACK, 2);
    }
}

void VoronoiQuadrilateralization::debugCheckGraphCompleteness()
{
    for (const node_t& node : graph.nodes)
    {
        if (!node.some_edge)
        {
            assert(false);
        }
    }
    for (const edge_t& edge : graph.edges)
    {
        if (!edge.twin || !edge.from || !edge.to)
        {
            assert(false);
        }
        assert(edge.next || edge.to->data.distance_to_boundary == 0);
        assert(edge.prev || edge.from->data.distance_to_boundary == 0);
    }
}

} // namespace arachne