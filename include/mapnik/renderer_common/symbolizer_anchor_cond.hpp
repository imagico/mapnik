/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2021 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#ifndef MAPNIK_RENDERER_COMMON_SYMBOLIZER_ANCHOR_COND_HPP
#define MAPNIK_RENDERER_COMMON_SYMBOLIZER_ANCHOR_COND_HPP

#include <mapnik/feature.hpp>
#include <mapnik/symbolizer.hpp>
#include <mapnik/label_collision_detector.hpp>

namespace mapnik {

template<typename Symbolizer, typename RendererType>
bool symbolizer_anchor_cond(Symbolizer const& sym,
                            mapnik::feature_impl& feature,
                            RendererType& common)
{
    std::string anchor_cond = get<std::string, keys::anchor_cond>(sym, feature, common.vars_);

    if (anchor_cond.empty() || common.detector_->has_anchor(anchor_cond))
        return true;
    else
        return false;
}

} // namespace mapnik

#endif // MAPNIK_RENDERER_COMMON_SYMBOLIZER_ANCHOR_COND_HPP
