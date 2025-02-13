// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// This file is copied from
// https://github.com/apache/kudu/blob/master/src/kudu/tablet/rowset_tree.h
// and modified by Doris

#pragma once

#include <boost/optional/optional.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "common/status.h"
#include "gutil/map-util.h"
#include "olap/rowset/rowset.h"
#include "util/slice.h"

namespace doris {

template <class Traits>
class IntervalTree;

struct RowsetIntervalTraits;
struct RowsetWithBounds;

// Used often enough, may as well typedef it.
typedef std::vector<RowsetSharedPtr> RowsetVector;

// Class which encapsulates the set of rowsets which are active for a given
// Tablet. This provides efficient lookup by key for Rowsets which may overlap
// that key range.
//
// Additionally, the rowset tree maintains information about the implicit
// intervals generated by the row sets (for instance, if a tablet has
// rowsets [0, 2] and [1, 3] it has three implicit contiguous intervals:
// [0, 1], [1, 2], and [2, 3].
class RowsetTree {
public:
    // An RSEndpoint is a POD which associates a rowset, an EndpointType
    // (either the START or STOP of an interval), and the key at which the
    // endpoint is located.
    enum EndpointType { START, STOP };
    struct RSEndpoint {
        RSEndpoint(RowsetSharedPtr rowset, uint32_t segment_id, EndpointType endpoint, Slice slice)
                : rowset_(rowset), segment_id_(segment_id), endpoint_(endpoint), slice_(slice) {}

        RowsetSharedPtr rowset_;
        uint32_t segment_id_;
        enum EndpointType endpoint_;
        Slice slice_;
    };

    RowsetTree();
    Status Init(const RowsetVector& rowsets);
    ~RowsetTree();

    // Return all Rowsets whose range may contain the given encoded key.
    //
    // The returned pointers are guaranteed to be valid at least until this
    // RowsetTree object is Reset().
    void FindRowsetsWithKeyInRange(const Slice& encoded_key,
                                   vector<std::pair<RowsetSharedPtr, int32_t>>* rowsets) const;

    // Call 'cb(rowset, index)' for each (rowset, index) pair such that
    // 'encoded_keys[index]' may be within the bounds of 'rowset'.
    //
    // See IntervalTree::ForEachIntervalContainingPoints for additional
    // information on the particular order in which the callback will be called.
    //
    // REQUIRES: 'encoded_keys' must be in sorted order.
    void ForEachRowsetContainingKeys(const std::vector<Slice>& encoded_keys,
                                     const std::function<void(RowsetSharedPtr, int)>& cb) const;

    // When 'lower_bound' is boost::none, it means negative infinity.
    // When 'upper_bound' is boost::none, it means positive infinity.
    // So the query interval can be one of below:
    //  [-OO, +OO)
    //  [-OO, upper_bound)
    //  [lower_bound, +OO)
    //  [lower_bound, upper_bound)
    void FindRowsetsIntersectingInterval(
            const std::optional<Slice>& lower_bound, const std::optional<Slice>& upper_bound,
            vector<std::pair<RowsetSharedPtr, int32_t>>* rowsets) const;

    const RowsetVector& all_rowsets() const { return all_rowsets_; }

    RowsetSharedPtr rs_by_id(RowsetId rs_id) const { return FindPtrOrNull(rs_by_id_, rs_id); }

    // Iterates over RowsetTree::RSEndpoint, guaranteed to be ordered and for
    // any rowset to appear exactly twice, once at its start slice and once at
    // its stop slice, equivalent to its GetBounds() values.
    const std::vector<RSEndpoint>& key_endpoints() const { return key_endpoints_; }

private:
    // Interval tree of the rowsets. Used to efficiently find rowsets which might contain
    // a probe row.
    std::unique_ptr<IntervalTree<RowsetIntervalTraits>> tree_;

    // Ordered map of all the interval endpoints, holding the implicit contiguous
    // intervals
    std::vector<RSEndpoint> key_endpoints_;

    // Container for all of the entries in tree_. IntervalTree does
    // not itself manage memory, so this provides a simple way to enumerate
    // all the entry structs and free them in the destructor.
    std::vector<RowsetWithBounds*> entries_;

    // All of the rowsets which were put in this RowsetTree.
    RowsetVector all_rowsets_;

    // The Rowsets in this RowsetTree, keyed by their id.
    std::unordered_map<RowsetId, RowsetSharedPtr, HashOfRowsetId> rs_by_id_;

    bool initted_;
};

void ModifyRowSetTree(const RowsetTree& old_tree, const RowsetVector& rowsets_to_remove,
                      const RowsetVector& rowsets_to_add, RowsetTree* new_tree);

} // namespace doris
