//Copyright (c) 2019 Ultimaker B.V.


#ifndef BEADING_ORDER_OPTIMIZER_H
#define BEADING_ORDER_OPTIMIZER_H

#include "utils/polygon.h"
#include "utils/ExtrusionSegment.h"
#include "utils/ExtrusionJunction.h"

namespace arachne
{

/*!
 * Connecting ExtrusionSegments together into chains / polygons
 */
class BeadingOrderOptimizer
{
public:
    static void optimize(const std::vector<ExtrusionSegment>& segments, std::vector<std::vector<std::vector<ExtrusionJunction>>>& polygons_per_index, std::vector<std::vector<std::vector<ExtrusionJunction>>>& polylines_per_index);
private:
    BeadingOrderOptimizer(const std::vector<ExtrusionSegment>& segments)
    : segments(segments)
    {
        even_polyline_end_points.reserve(segments.size());
        odd_polyline_end_points.reserve(segments.size());
    }

    struct Polyline
    {
        coord_t inset_idx;
        std::list<ExtrusionJunction> junctions;
        Polyline(coord_t inset_idx)
        : inset_idx(inset_idx)
        {}
    };

    struct PolylineEndRef
    {
        coord_t inset_idx;
        std::list<Polyline>::iterator polyline;
        bool front;
        PolylineEndRef(coord_t inset_idx, std::list<Polyline>::iterator polyline, bool front)
        : inset_idx(inset_idx)
        , polyline(polyline)
        , front(front)
        {}
    };

    const std::vector<ExtrusionSegment>& segments;

    std::list<Polyline> even_polylines;
    std::list<Polyline> odd_polylines; // keep odd single bead segments separate so that polygon segments can combine together into polygons
    std::unordered_map<Point, PolylineEndRef> even_polyline_end_points;
    std::unordered_map<Point, PolylineEndRef> odd_polyline_end_points;

    void connect(std::vector<std::vector<std::vector<ExtrusionJunction>>>& result_polygons_per_index);
    
    
    
    void transferUnconnectedPolylines(std::vector<std::vector<std::vector<ExtrusionJunction>>>& result_polygons_per_index, std::vector<std::vector<std::vector<ExtrusionJunction>>>& result_polylines_per_index);
    
    void debugCheck();
};




} // namespace arachne
#endif // BEADING_ORDER_OPTIMIZER_H