//Copyright (c) 2020 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.


#ifndef CENTER_DEVIATION_BEADING_STRATEGY_H
#define CENTER_DEVIATION_BEADING_STRATEGY_H

#include "BeadingStrategy.h"

namespace cura
{

/*!
 * This beading strategy makes the deviation in the thickness of the part
 * entirely compensated by the innermost wall.
 *
 * The outermost walls all use the ideal width, as far as possible.
 */
class CenterDeviationBeadingStrategy : public BeadingStrategy
{
    coord_t overfill_bound; // Amount of overfill before the two innermost beads are replaced by a single in the middle.
    coord_t underfill_bound; // Amount of underfil before a single bead in the middle is placed
public:
    CenterDeviationBeadingStrategy(const coord_t pref_bead_width_outer, const coord_t pref_bead_width_inner, float transitioning_angle, float min_diameter = 0.8, float max_diameter = 1.25)
    : BeadingStrategy(pref_bead_width_outer, pref_bead_width_inner, std::max(pref_bead_width_outer, pref_bead_width_inner) / 2, transitioning_angle)
    , overfill_bound(std::max(pref_bead_width_outer, pref_bead_width_inner) - min_diameter * std::min(pref_bead_width_outer, pref_bead_width_inner))
    , underfill_bound(max_diameter * std::max(pref_bead_width_outer, pref_bead_width_inner) - std::min(pref_bead_width_outer, pref_bead_width_inner))
    {
        name = "CenterDeviationBeadingStrategy";
    }
    virtual ~CenterDeviationBeadingStrategy() override
    {}
    Beading compute(coord_t thickness, coord_t bead_count) const override;
    coord_t getOptimalThickness(coord_t bead_count) const override;
    coord_t getTransitionThickness(coord_t lower_bead_count) const override;
    coord_t getOptimalBeadCount(coord_t thickness) const override;
};

} // namespace cura
#endif // CENTER_DEVIATION_BEADING_STRATEGY_H