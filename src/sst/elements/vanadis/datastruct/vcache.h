// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_CACHE
#define _H_VANADIS_CACHE

#include <cstdint>
#include <list>
#include <type_traits>
#include <unordered_map>

namespace SST {
namespace Vanadis {

enum class VanadisCacheRecordDeletion {
    VANADIS_NO_DELETION,
    VANADIS_PERFORM_DELETE,
    VANADIS_PERFORM_DELETE_ARRAY
};

template <typename I, typename T, SST::Vanadis::VanadisCacheRecordDeletion D>
class VanadisCache {

    // Cache entry type
    using ListIt = typename std::list<I>::iterator;
    struct Entry {
        T* value;
        ListIt lru_it;
    };

public:
    VanadisCache(const size_t cache_entries) : max_entries_(cache_entries) {
        data_values_.reserve(max_entries_);
    }

    ~VanadisCache() { clear(); }

    void clear() {
        for ( auto& kv : data_values_ ) {
            destroy(kv.second.value);
        }

        lru_.clear();
        data_values_.clear();
    }

    void reset() {
        clear();
        data_values_.reserve(max_entries_);
    }

    bool contains(const I& value) const { return (data_values_.find(value) != data_values_.end()); }

    T* find(const I& key) {
        auto it = data_values_.find(key);
        if (it == data_values_.end()) return nullptr;
        lru_.splice(lru_.begin(), lru_, it->second.lru_it);
        return it->second.value;
    }

    void store(const I& key, T* value) {
        auto it = data_values_.find(key);

        if ( LIKELY(it != data_values_.end()) ) {
            lru_.splice(lru_.begin(), lru_, it->second.lru_it);
            destroy(it->second.value);
            it->second.value = value;
            return;
        }

        makeSpace();
        lru_.push_front(key);
        data_values_.emplace(key, Entry{value, lru_.begin()});
    }

    bool touch(const I& key) {
        auto it = data_values_.find(key);
        if (it == data_values_.end()) return false;
        lru_.splice(lru_.begin(), lru_, it->second.lru_it);
        return true;
    }

    size_t size() const { return data_values_.size(); }
    size_t capacity() const { return max_entries_; }

private:
    void makeSpace() {
        // if we aren't full yet, then keep entries otherwise we will throw away
        if ( UNLIKELY(lru_.size() < max_entries_) ) {
            return;
        }

        const I& remove_key = lru_.back();
        auto it = data_values_.find(remove_key);
        destroy(it->second.value);

        data_values_.erase(it);
        lru_.pop_back();
    }

    static void destroy(T*& value) {
        if constexpr( D == VanadisCacheRecordDeletion::VANADIS_PERFORM_DELETE ) {
            delete value;
        } else if constexpr ( D == VanadisCacheRecordDeletion::VANADIS_PERFORM_DELETE_ARRAY ) {
            delete [] value;
        }
    }

    const size_t max_entries_;
    std::list<I> lru_;
    std::unordered_map<I, Entry> data_values_;
};

} // namespace Vanadis
} // namespace SST

#endif
