/*
 * Copyright 2019-2021 Azavea
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CACHE_HPP__
#define __CACHE_HPP__

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <pthread.h>

#include "types.hpp"
#include "locked_dataset.hpp"

/*
 * A class implementing an LRU cache of locked_dataset objects.  It is
 * "flat" in the sense of being implemented on top of an array instead
 * of map (this works because the size will always be ≤ the
 * per-process file descriptor limit).
 */
class flat_lru_cache
{
public:
    typedef uri_options_t key_t;
    typedef locked_dataset value_t;
    typedef uint64_t atime_t;
    typedef std::atomic<atime_t> atomic_atime_t;
    typedef std::vector<locked_dataset *> return_list_t;

    /*
     * Constructor
     *
     * @param capacity The maximum number of objects that the cache can hold
     */
    flat_lru_cache(size_t capacity)
        : m_tags(std::vector<size_t>(capacity)),
          m_atimes(std::vector<atomic_atime_t>(capacity)),
          m_values(std::vector<value_t>(capacity)),
          m_time(0),
          m_capacity(capacity),
          m_size(0),
          m_cache_lock(PTHREAD_RWLOCK_INITIALIZER)
    {
        clear();
    }

    flat_lru_cache(const flat_lru_cache &rhs)
        : m_tags(std::vector<size_t>(rhs.m_capacity)),
          m_atimes(std::vector<atomic_atime_t>(rhs.m_capacity)),
          m_values(std::vector<value_t>(rhs.m_capacity)),
          m_time(rhs.m_time.load()),
          m_capacity(rhs.m_capacity),
          m_size(rhs.m_size),
          m_cache_lock(PTHREAD_RWLOCK_INITIALIZER)
    {
        clear();
    }

    ~flat_lru_cache()
    {
    }

    /*
     * The capacity of the cache.
     */
    size_t capacity() const
    {
        return m_capacity;
    }

    /*
     * The number of items currently in the cache.
     */
    size_t size() const
    {
        return std::min(m_capacity, m_size);
    }

    /*
     * Clear the cache.
     */
    void clear()
    {
        pthread_rwlock_wrlock(&m_cache_lock);
        for (size_t i = 0; i < capacity(); ++i)
        {
            m_tags[i] = 0;
            m_atimes[i] = 0;
            m_values[i].lock_for_deletion();
            m_values[i] = locked_dataset();
            m_size = 0;
        }
        pthread_rwlock_unlock(&m_cache_lock);
    }

    /*
     * Does the cache contain a value with the given key?
     *
     * @param key A uri ⨯ options pair
     * @return True if there is value for the key, false otherwise
     */
    bool contains(const uri_options_t &key) const
    {
        auto h = uri_options_hash_t();
        auto tag = h(key);

        pthread_rwlock_rdlock(&m_cache_lock);
        for (size_t i = 0; i < capacity(); ++i)
        {
            if (m_tags[i] == tag)
            {
                if (m_values[i] == key)
                {
                    pthread_rwlock_unlock(&m_cache_lock);
                    return true;
                }
            }
        }
        pthread_rwlock_unlock(&m_cache_lock);
        return false;
    }

    /*
     * The number of values in the cache associated with the given
     * key.
     *
     * @param key A uri ⨯ options pair
     * @return The number of values (could be zero)
     */
    size_t count(const uri_options_t &key) const
    {
        auto h = uri_options_hash_t();
        auto tag = h(key);
        size_t result = 0;

        pthread_rwlock_rdlock(&m_cache_lock);
        for (size_t i = 0; i < capacity(); ++i)
        {
            if (m_tags[i] == tag && m_values[i] == key)
            {
                result += 1;
            }
        }
        pthread_rwlock_unlock(&m_cache_lock);
        return result;
    }

    /*
     * Get a list of values associated with the given key.  It is
     * possible that not-already-present values will be created, and
     * it is also possible that the list will be empty.
     *
     * @param key A uri ⨯ options pair
     * @param copies The number of datasets to try to return: if
     *               positive try really hard to return this many, if
     *               negative try reasonably hard to return the
     *               negative of this many
     * @return A vector of values associated with the key
     */
    return_list_t get(const uri_options_t &key, int copies = 1)
    {
        auto h = uri_options_hash_t();
        auto tag = h(key);
        auto return_list = return_list_t();

        // Search the cache for existing values matching the key
        pthread_rwlock_rdlock(&m_cache_lock);
        for (size_t i = 0; i < capacity(); ++i)
        {
            if (m_tags[i] == tag && m_values[i] == key)
            {
                auto &ld = m_values[i];
                ld.inc();
                return_list.push_back(&ld);
                // m_atimes[i] is not guranteed to increase
                // monotonically, but that is acceptable in this
                // situation.  See
                // https://stackoverflow.com/questions/16190078/how-to-atomically-update-a-maximum-value
                // for information on how to do it strictly
                m_atimes[i] = static_cast<atime_t>(++m_time);
            }
        }
        pthread_rwlock_unlock(&m_cache_lock);

        // If the number of values found above is below the
        // hard-request number (which is `copies` if that value is
        // positive or 1 otherwise), then try hard to create enough
        // new datasets to achieve that number.
        int copies2 = copies < 0 ? 1 : copies;
        if ((copies2 > 0) && (return_list.size() < static_cast<unsigned int>(copies2)))
        {
            pthread_rwlock_wrlock(&m_cache_lock);
            for (size_t i = copies2 - return_list.size(); i > 0; --i)
            {
                auto ld = insert(tag, key);
                if (ld != nullptr)
                {
                    ld->inc();
                    return_list.push_back(ld);
                }
            }
            pthread_rwlock_unlock(&m_cache_lock);
        }
        // If the number of values found above is below the
        // soft-request number (which is the negative of `copies` if
        // that value is negative), then try to create enough datasets
        // to achieve that number.
        else if ((copies <= 0) && (return_list.size() < static_cast<unsigned int>(-copies)))
        {
            if (pthread_rwlock_trywrlock(&m_cache_lock) == 0)
            {
                for (size_t i = -copies - return_list.size(); i > 0; --i)
                {
                    auto ld = insert(tag, key);
                    if (ld != nullptr)
                    {
                        ld->inc();
                        return_list.push_back(ld);
                    }
                }
                pthread_rwlock_unlock(&m_cache_lock);
            }
        }

        return return_list;
    }

private:
    /*
     * Insert a new entry into the cache.
     *
     * @param tag The tag of the new entry within the cache
     * @param key The key of the new entry within the cache
     * @return A valid pointer to the newly-created object or nullptr
     */
    locked_dataset *insert(size_t tag, const uri_options_t &key)
    {
        int best_index = -1;
        atime_t best_atime = -1; // sic

        // Scan through the cache to find the least recently used
        // entry (for eviction)
        for (size_t i = 0; i < capacity(); ++i)
        {
            if (m_atimes[i] < best_atime && m_values[i].lock_for_deletion())
            {
                if (best_index != -1)
                {
                    m_values[best_index].unlock_for_nondeletion();
                }
                best_index = i;
                best_atime = m_atimes[i];
            }
        }
        // If an eviction candidate has been found, then create a new
        // value (locked_dataset) and put it into the cache if it is
        // valid.  Invalid datasets can be caused by such things as
        // bad URIs and low system resources
        if (best_index != -1)
        {
            auto ds = locked_dataset(key);

            if (ds.valid())
            {
                m_tags[best_index] = tag;
                m_atimes[best_index] = ++m_time;
                m_values[best_index] = std::move(ds);
                m_size++;
                return &(m_values[best_index]);
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
    }

private:
    std::vector<size_t> m_tags;
    std::vector<atomic_atime_t> m_atimes;
    std::vector<value_t> m_values;
    atomic_atime_t m_time;
    size_t m_capacity;
    size_t m_size;
    mutable pthread_rwlock_t m_cache_lock;
};

#endif // __CACHE_HPP__
