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

#ifndef MAPNIK_LABEL_COLLISION_DETECTOR_HPP
#define MAPNIK_LABEL_COLLISION_DETECTOR_HPP

// mapnik
#include <mapnik/debug.hpp>
#include <mapnik/quad_tree.hpp>
#include <mapnik/util/noncopyable.hpp>
#include <mapnik/value/types.hpp>

#include <mapnik/warning.hpp>
MAPNIK_DISABLE_WARNING_PUSH
#include <mapnik/warning_ignore.hpp>
#include <unicode/unistr.h>
#include <boost/algorithm/string.hpp>
MAPNIK_DISABLE_WARNING_POP

// stl
#include <vector>
#include <set>

namespace mapnik {
// this needs to be tree structure
// as a proof of a concept _only_ we use sequential scan

struct label_collision_detector
{
    using label_placements = std::vector<box2d<double>>;

    bool has_placement(box2d<double> const& box)
    {
        for (auto const& label : labels_)
        {
            if (label.intersects(box))
                return false;
        }
        labels_.push_back(box);
        return true;
    }
    void clear() { labels_.clear(); }

  private:

    label_placements labels_;
};

// quad_tree based label collision detector
class label_collision_detector2 : util::noncopyable
{
    using tree_t = quad_tree<box2d<double>>;
    tree_t tree_;

  public:

    explicit label_collision_detector2(box2d<double> const& extent)
        : tree_(extent)
    {}

    bool has_placement(box2d<double> const& box)
    {
        tree_t::query_iterator itr = tree_.query_in_box(box);
        tree_t::query_iterator end = tree_.query_end();
        for (; itr != end; ++itr)
        {
            if (itr->get().intersects(box))
                return false;
        }
        tree_.insert(box, box);
        return true;
    }

    void clear() { tree_.clear(); }
};

// quad_tree based label collision detector with seperate check/insert
class label_collision_detector3 : util::noncopyable
{
    using tree_t = quad_tree<box2d<double>>;
    tree_t tree_;

  public:

    explicit label_collision_detector3(box2d<double> const& extent)
        : tree_(extent)
    {}

    bool has_placement(box2d<double> const& box)
    {
        tree_t::query_iterator itr = tree_.query_in_box(box);
        tree_t::query_iterator end = tree_.query_end();

        for (; itr != end; ++itr)
        {
            if (itr->get().intersects(box))
                return false;
        }
        return true;
    }

    void insert(box2d<double> const& box) { tree_.insert(box, box); }

    void clear() { tree_.clear(); }
};

// quad tree based label collision detector so labels dont appear within a given distance
class label_collision_detector4 : util::noncopyable
{
  public:
    struct label
    {
        label(box2d<double> const& b)
            : box(b)
            , text()
        {}
        label(box2d<double> const& b, mapnik::value_unicode_string const& t)
            : box(b)
            , text(t)
        {}
        label(box2d<double> const& b, std::string const& a)
            : box(b)
            , text()
        {
            boost::split(anchors, a, boost::is_any_of(","));
        }
        label(box2d<double> const& b, mapnik::value_unicode_string const& t, std::string const& a)
            : box(b)
            , text(t)
        {
            boost::split(anchors, a, boost::is_any_of(","));
        }
        box2d<double> box;
        mapnik::value_unicode_string text;
        std::set<std::string> anchors;
    };

  private:
    using tree_t = quad_tree<label>;
    tree_t tree_;

    std::set<std::string> anchors_;

  public:
    using query_iterator = tree_t::query_iterator;

    explicit label_collision_detector4(box2d<double> const& _extent)
        : tree_(_extent)
    {}

    bool has_placement(box2d<double> const& box)
    {
        tree_t::query_iterator tree_itr = tree_.query_in_box(box);
        tree_t::query_iterator tree_end = tree_.query_end();

        for (; tree_itr != tree_end; ++tree_itr)
        {
            if (tree_itr->get().box.intersects(box))
                return false;
        }

        return true;
    }

    bool has_placement(box2d<double> const& box, std::string const& anchor_exclude)
    {
        tree_t::query_iterator tree_itr = tree_.query_in_box(box);
        tree_t::query_iterator tree_end = tree_.query_end();

        if (!anchor_exclude.empty())
            MAPNIK_LOG_ERROR(label_collision_detector4) << "has_placement anchor_exclude: " << anchor_exclude;

        for (; tree_itr != tree_end; ++tree_itr)
        {
            bool do_exclude = false;
            if (!anchor_exclude.empty())
            {
                std::vector<std::string> conds;
                boost::split(conds, anchor_exclude, boost::is_any_of(","));

                for (auto & cond : conds)
                {
                    boost::algorithm::trim(cond);

                    if (tree_itr->get().anchors.count(cond)!=0)
                    {
                        do_exclude = true;
                        break;
                    }
                }
            }

            if (!do_exclude)
                if (tree_itr->get().box.intersects(box))
                {
                    std::string s;
                    for (auto const& a : tree_itr->get().anchors)
                    {
                      s += a;
                      s += ",";
                    }
                    s.pop_back();
                    if (!anchor_exclude.empty())
                        MAPNIK_LOG_ERROR(label_collision_detector4) << "has_placement collision: " << anchor_exclude << "/" << s << ";";
                    else
                        MAPNIK_LOG_ERROR(label_collision_detector4) << "has_placement collision: " << s << ";";
                    return false;
                }
        }

        return true;
    }

    bool has_placement(box2d<double> const& box, double margin)
    {
        box2d<double> const& margin_box =
          (margin > 0
             ? box2d<double>(box.minx() - margin, box.miny() - margin, box.maxx() + margin, box.maxy() + margin)
             : box);

        tree_t::query_iterator tree_itr = tree_.query_in_box(margin_box);
        tree_t::query_iterator tree_end = tree_.query_end();

        for (; tree_itr != tree_end; ++tree_itr)
        {
            if (tree_itr->get().box.intersects(margin_box))
            {
                return false;
            }
        }
        return true;
    }

    bool has_placement(box2d<double> const& box, double margin, std::string const& anchor_exclude)
    {
        box2d<double> const& margin_box =
          (margin > 0
             ? box2d<double>(box.minx() - margin, box.miny() - margin, box.maxx() + margin, box.maxy() + margin)
             : box);

        tree_t::query_iterator tree_itr = tree_.query_in_box(margin_box);
        tree_t::query_iterator tree_end = tree_.query_end();

        for (; tree_itr != tree_end; ++tree_itr)
        {
            bool do_exclude = false;
            if (!anchor_exclude.empty())
            {
                std::vector<std::string> conds;
                boost::split(conds, anchor_exclude, boost::is_any_of(","));

                for (auto & cond : conds)
                {
                    boost::algorithm::trim(cond);

                    if (tree_itr->get().anchors.count(cond)!=0)
                    {
                        do_exclude = true;
                        break;
                    }
                }
            }

            if (!do_exclude)
                if (tree_itr->get().box.intersects(margin_box))
                {
                    return false;
                }
        }
        return true;
    }

    bool has_placement(box2d<double> const& box,
                       double margin,
                       mapnik::value_unicode_string const& text,
                       double repeat_distance)
    {
        // Don't bother with any of the repeat checking unless the repeat distance is greater than the margin
        if (repeat_distance <= margin)
        {
            return has_placement(box, margin);
        }

        box2d<double> repeat_box(box.minx() - repeat_distance,
                                 box.miny() - repeat_distance,
                                 box.maxx() + repeat_distance,
                                 box.maxy() + repeat_distance);

        box2d<double> const& margin_box =
          (margin > 0
             ? box2d<double>(box.minx() - margin, box.miny() - margin, box.maxx() + margin, box.maxy() + margin)
             : box);

        tree_t::query_iterator tree_itr = tree_.query_in_box(repeat_box);
        tree_t::query_iterator tree_end = tree_.query_end();

        for (; tree_itr != tree_end; ++tree_itr)
        {
            if (tree_itr->get().box.intersects(margin_box) ||
                (text == tree_itr->get().text && tree_itr->get().box.intersects(repeat_box)))
            {
                return false;
            }
        }

        return true;
    }

    bool has_placement(box2d<double> const& box,
                       double margin,
                       mapnik::value_unicode_string const& text,
                       double repeat_distance,
                       std::string const& anchor_exclude)
    {
        // Don't bother with any of the repeat checking unless the repeat distance is greater than the margin
        if (repeat_distance <= margin)
        {
            return has_placement(box, margin, anchor_exclude);
        }

        box2d<double> repeat_box(box.minx() - repeat_distance,
                                 box.miny() - repeat_distance,
                                 box.maxx() + repeat_distance,
                                 box.maxy() + repeat_distance);

        box2d<double> const& margin_box =
          (margin > 0
             ? box2d<double>(box.minx() - margin, box.miny() - margin, box.maxx() + margin, box.maxy() + margin)
             : box);

        tree_t::query_iterator tree_itr = tree_.query_in_box(repeat_box);
        tree_t::query_iterator tree_end = tree_.query_end();

        for (; tree_itr != tree_end; ++tree_itr)
        {
            bool do_exclude = false;
            if (!anchor_exclude.empty())
            {
                std::vector<std::string> conds;
                boost::split(conds, anchor_exclude, boost::is_any_of(","));

                for (auto & cond : conds)
                {
                    boost::algorithm::trim(cond);

                    if (tree_itr->get().anchors.count(cond)!=0)
                    {
                        do_exclude = true;
                        break;
                    }
                }
            }

            if (!do_exclude)
                if (tree_itr->get().box.intersects(margin_box) ||
                    (text == tree_itr->get().text && tree_itr->get().box.intersects(repeat_box)))
                {
                    return false;
                }
        }

        return true;
    }

    bool has_anchor(std::string const& anchor_cond)
    {
        if (anchor_cond.empty()) return true;

        MAPNIK_LOG_ERROR(label_collision_detector4) << "has_anchor: " << anchor_cond;

        std::vector<std::string> conds;
        boost::split(conds, anchor_cond, boost::is_any_of(","));

        for (auto & cond : conds)
        {
            boost::algorithm::trim(cond);
            if (cond.front() == '!')
            {
                if (anchors_.count(cond.substr(1))!=0)
                {
                    //MAPNIK_LOG_ERROR(label_collision_detector4) << "has_anchor: " << cond << ": false";
                    return false;
                }
            }
            else
            {
                if (anchors_.count(cond)==0)
                {
                    //MAPNIK_LOG_ERROR(label_collision_detector4) << "has_anchor: " << cond << ": false";
                    return false;
                }
            }
        }
        return true;
    }

    void insert(box2d<double> const& box)
    {
        if (tree_.extent().intersects(box))
        {
            //MAPNIK_LOG_ERROR(label_collision_detector4) << "insert box";
            tree_.insert(label(box), box);
        }
    }

    void insert(box2d<double> const& box, mapnik::value_unicode_string const& text)
    {
        if (tree_.extent().intersects(box))
        {
            //MAPNIK_LOG_ERROR(label_collision_detector4) << "insert text";
            tree_.insert(label(box, text), box);
        }
    }

    void insert(box2d<double> const& box, std::string const& anchor)
    {
        if (tree_.extent().intersects(box))
        {
            //MAPNIK_LOG_ERROR(label_collision_detector4) << "insert anchor: " << anchor;
            tree_.insert(label(box, anchor), box);
        }
    }

    void insert(box2d<double> const& box, mapnik::value_unicode_string const& text, std::string const& anchor)
    {
        if (tree_.extent().intersects(box))
        {
            //MAPNIK_LOG_ERROR(label_collision_detector4) << "insert text anchor: " << anchor;
            tree_.insert(label(box, text, anchor), box);
        }
    }

    void add_anchor(std::string const& anchor)
    {
        MAPNIK_LOG_ERROR(label_collision_detector4) << "add_anchor: " << anchor;

        std::vector<std::string> anchors;
        boost::split(anchors, anchor, boost::is_any_of(","));

        for (auto & a : anchors)
        {
            boost::algorithm::trim(a);
            // not sure how useful it is to remove anchors - but there is no reason why this should be disallowed
            // note this does not remove the corresponding entry from the collision detector, the anchor will still remain in there
            if (a.front() == '!')
            {
                //MAPNIK_LOG_ERROR(label_collision_detector4) << "add_anchor: erase " << a ;
                anchors_.erase(a.substr(1));
            }
            else
            {
                //MAPNIK_LOG_ERROR(label_collision_detector4) << "add_anchor: insert " << a ;
                anchors_.insert(a);
            }
        }
    }

    void clear() { tree_.clear(); anchors_.clear(); }

    box2d<double> const& extent() const { return tree_.extent(); }

    query_iterator begin() { return tree_.query_in_box(extent()); }
    query_iterator end() { return tree_.query_end(); }
};
} // namespace mapnik

#endif // MAPNIK_LABEL_COLLISION_DETECTOR_HPP
