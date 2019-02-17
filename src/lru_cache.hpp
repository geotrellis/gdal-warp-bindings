//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_LRU_CACHE_HPP
#define BOOST_COMPUTE_DETAIL_LRU_CACHE_HPP

#include <map>
#include <list>
#include <utility>
#include <pthread.h>
#include "types.hpp"
#include "locked_dataset.hpp"

// a cache which evicts the least recently used item when it is full
class lru_cache
{
  public:
    typedef std::list<uri_options_t> list_t;
    typedef std::map<uri_options_t, std::pair<locked_dataset, list_t::iterator>> map_t;

    lru_cache(size_t capacity)
        : m_capacity(capacity), m_map_lock(PTHREAD_RWLOCK_INITIALIZER), m_list_lock(PTHREAD_RWLOCK_INITIALIZER)
    {
    }

    ~lru_cache()
    {
    }

    size_t size()
    {
        pthread_rwlock_rdlock(&m_map_lock);
        auto retval = m_map.size();
        pthread_rwlock_unlock(&m_map_lock);
        return retval;
    }

    size_t capacity()
    {
        return m_capacity;
    }

    bool empty()
    {
        pthread_rwlock_wrlock(&m_map_lock);
        auto retval = m_map.empty();
        pthread_rwlock_unlock(&m_map_lock);
        return retval;
    }

    bool contains(const uri_options_t &key)
    {
        pthread_rwlock_rdlock(&m_map_lock);
        auto retval = m_map.find(key) != m_map.end();
        pthread_rwlock_unlock(&m_map_lock);
        return retval;
    }

    const locked_dataset &get(const uri_options_t &key)
    {
        // lookup value in the cache
        pthread_rwlock_wrlock(&m_map_lock);
        map_t::iterator i = m_map.find(key);
        if (i == m_map.end())
        {
            // value not in cache
            auto value = locked_dataset(key);
            insert(key, value);
            i = m_map.find(key);
        }

        // return the value, but first update its place in the most
        // recently used list
        pthread_rwlock_wrlock(&m_list_lock);
        list_t::iterator j = i->second.second;
        if (j != m_list.begin())
        {
            // move item to the front of the most recently used list
            m_list.erase(j);
            m_list.push_front(key);
            j = m_list.begin();

            // update iterator in map
            const locked_dataset &value = i->second.first;
            m_map[key] = std::make_pair(value, j);
            pthread_rwlock_unlock(&m_list_lock);
            pthread_rwlock_unlock(&m_map_lock);

            // return the value
            return value;
        }
        else
        {
            // the item is already at the front of the most recently
            // used list so just return it
            pthread_rwlock_unlock(&m_list_lock);
            pthread_rwlock_unlock(&m_map_lock);
            return i->second.first;
        }
    }

    void clear()
    {
        pthread_rwlock_wrlock(&m_map_lock);
        m_map.clear();
        pthread_rwlock_unlock(&m_map_lock);
        pthread_rwlock_wrlock(&m_list_lock);
        m_list.clear();
        pthread_rwlock_unlock(&m_list_lock);
    }

  private:
    // There is only one path to this function.  It is only ever
    // called when the map lock is held.
    void insert(const uri_options_t &key, locked_dataset &value)
    {
        pthread_rwlock_wrlock(&m_list_lock);
        map_t::iterator i = m_map.find(key);
        if (i == m_map.end())
        {
            // insert item into the cache, but first check if it is full
            if (m_map.size() >= m_capacity)
            {
                // cache is full, evict the least recently used item
                evict();
            }

            // insert the new item
            m_list.push_front(key);
            m_map[key] = std::move(std::make_pair(std::move(value), m_list.begin()));
        }
        pthread_rwlock_unlock(&m_list_lock);
    }

    // There is only one path to this function.  It is only ever
    // called when the map lock and list lock are both held.
    void evict()
    {
        // evict item from the end of most recently used list
        list_t::iterator i = --m_list.end();
        m_map.erase(*i);
        m_list.erase(i);
    }

  private:
    map_t m_map;
    list_t m_list;
    size_t m_capacity;
    pthread_rwlock_t m_map_lock;
    pthread_rwlock_t m_list_lock;
};

#endif // BOOST_COMPUTE_DETAIL_LRU_CACHE_HPP
