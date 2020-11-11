#ifndef DG_HASH_MAP_H_
#define DG_HASH_MAP_H_

#include <unordered_map>

namespace dg {

namespace {
template <typename Key, typename Val, typename Impl>
class HashMapImpl {
    Impl _map;

public:
    using iterator = typename Impl::iterator;
    using const_iterator = typename Impl::const_iterator;

    bool insert(const Key& k, Val& v) {
        auto it = _map.insert(k, v);
        return it.second;
    }

    Val& operator[](const Key& k) { return _map[k]; }
    Val *get(const Key& k) {
        auto it = _map.find(k);
        if (it != _map.end())
            return &it->second;
        return nullptr;
    }

    auto begin() -> decltype(_map.begin()) { return _map.begin(); }
    auto end() -> decltype(_map.end()) { return _map.end(); }
    auto begin() const -> decltype(_map.begin()) { return _map.begin(); }
    auto end() const -> decltype(_map.end()) { return _map.end(); }
};
}

template <typename Key, typename Val>
class STLHashMap : public HashMapImpl<Key, Val, std::unordered_map<Key, Val>> {};


// unordered_map that caches last several accesses
// XXX: use a different implementation than std::unordered_map
template <typename Key, typename T, unsigned CACHE_SIZE=4U>
class CachingHashMap : public std::unordered_map<Key, T> {
    std::pair<Key, T*> _cache[CACHE_SIZE];
    unsigned _insert_pos{0};
    unsigned _last{0};

    T *_get_from_cache(const Key& key) {
        if (_last > 0) {
            for (unsigned i = 0; i < _last; ++i) {
                if (_cache[i].first == key)
                    return _cache[i].second;
            }
        }
        return nullptr;
    }

    void _insert_to_cache(const Key& key, T *v) {
        _last = std::min(_last + 1, CACHE_SIZE);
        _cache[_insert_pos] = {key, v};
        _insert_pos = (_insert_pos + 1) % CACHE_SIZE;

        assert(_insert_pos < CACHE_SIZE);
        assert(_last <= CACHE_SIZE);
    }

    void _invalidate_cache() {
        _last = 0;
        _insert_pos = 0;
    }

public:
    using iterator = typename std::unordered_map<Key, T>::iterator;
    using const_iterator = typename std::unordered_map<Key, T>::const_iterator;

    T& operator[](const Key& key) {
        if (auto *v = _get_from_cache(key)) {
            return *v;
        }

        auto& ret = std::unordered_map<Key, T>::operator[](key);
        _insert_to_cache(key, &ret);
        return ret;
    }

    iterator erase(const_iterator pos) {
        _invalidate_cache();
        return std::unordered_map<Key, T>::erase(pos);
    }

    iterator erase(const_iterator first, const_iterator last) {
        _invalidate_cache();
        return std::unordered_map<Key, T>::erase(first, last);
    }
};
    
} // namespace dg

#ifdef HAS_GOOGLE_HASH
#include <sparsehash/sparse_hash_map>

namespace dg {

template <typename Key, typename Val>
class SparseHash {
};

} // namespace dg
#endif

namespace dg {

#ifdef HAS_GOOGLE_HASH
template <typename Key, typename Val>
using HashMap = SparseHash<Key, Val>;
#else
template <typename Key, typename Val>
using HashMap = STLHashMap<Key, Val>;
#endif

} // namespace dg

#endif
