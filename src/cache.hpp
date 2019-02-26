/*
 * Copyright 2019 Azavea
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

#include <vector>
#include <cstddef>
#include <cstdint>

#include <pthread.h>

#include "types.hpp"
#include "locked_dataset.hpp"

class lru_cache
{
  public:
    typedef uri_options_t key_t;
    typedef locked_dataset value_t;
    typedef uint64_t atime_t;
    typedef std::vector<locked_dataset *> return_list_t;

    lru_cache(size_t capacity, size_t copies = 1)
        : m_tags(std::vector<size_t>(capacity)),
          m_atimes(std::vector<atime_t>(capacity)),
          m_keys(std::vector<key_t>(capacity)),
          m_values(std::vector<value_t>(capacity)),
          m_capacity(capacity),
          m_copies(copies),
          m_tag_lock(PTHREAD_RWLOCK_INITIALIZER),
          m_atimes_lock(PTHREAD_RWLOCK_INITIALIZER),
          m_keys_lock(PTHREAD_RWLOCK_INITIALIZER),
          m_values_lock(PTHREAD_RWLOCK_INITIALIZER),
          m_time(0),
          m_size(0)
    {
        clear();
    }

    ~lru_cache()
    {
    }

    size_t capacity() const
    {
        return m_capacity;
    }

    size_t copies() const
    {
        return m_copies;
    }

    size_t size() const
    {
        return std::min(m_capacity, m_size);
    }

    void clear()
    {
        pthread_rwlock_wrlock(&m_values_lock);
        for (size_t i = 0; i < capacity(); ++i)
        {
            m_tags[i] = 0;
            m_atimes[i] = 0;
            m_keys[i] = uri_options_t();
            m_values[i] = locked_dataset();
            m_size = 0;
        }
        pthread_rwlock_unlock(&m_values_lock);
    }

    bool contains(const uri_options_t &key) const
    {
        auto h = uri_options_hash_t();
        auto tag = h(key);

        pthread_rwlock_rdlock(&m_tag_lock);
        pthread_rwlock_rdlock(&m_keys_lock);
        for (size_t i = 0; i < capacity(); ++i)
        {
            if (m_tags[i] == tag)
            {
                if (m_keys[i] == key)
                {
                    pthread_rwlock_unlock(&m_keys_lock);
                    pthread_rwlock_unlock(&m_tag_lock);
                    return true;
                }
            }
        }
        pthread_rwlock_unlock(&m_keys_lock);
        pthread_rwlock_unlock(&m_tag_lock);
        return false;
    }

    size_t count(const uri_options_t &key) const
    {
        auto h = uri_options_hash_t();
        auto tag = h(key);
        size_t result = 0;

        pthread_rwlock_rdlock(&m_tag_lock);
        pthread_rwlock_rdlock(&m_keys_lock);
        for (size_t i = 0; i < capacity(); ++i)
        {
            if (m_tags[i] == tag && m_keys[i] == key)
            {
                result += 1;
            }
        }
        pthread_rwlock_unlock(&m_keys_lock);
        pthread_rwlock_unlock(&m_tag_lock);
        return result;
    }

    return_list_t get(const uri_options_t &key)
    {
        auto h = uri_options_hash_t();
        auto tag = h(key);
        auto return_list = return_list_t();
        auto current_time = ++m_time; // XXX

        pthread_rwlock_rdlock(&m_tag_lock);
        pthread_rwlock_rdlock(&m_keys_lock);
        for (size_t i = 0; i < capacity(); ++i)
        {
            if (m_tags[i] == tag && m_keys[i] == key)
            {
                auto &value = m_values[i];
                value.inc();
                return_list.push_back(&value);
                m_atimes[i] = current_time;
            }
        }
        pthread_rwlock_unlock(&m_keys_lock);
        pthread_rwlock_unlock(&m_tag_lock);

        if (return_list.size() < 1) // Try hard to return at least one
        {
            pthread_rwlock_wrlock(&m_values_lock);
            auto ld = insert(tag, key);
            if (ld != nullptr)
            {
                ld->inc();
                return_list.push_back(ld);
            }
            pthread_rwlock_unlock(&m_values_lock);
        }

        if (return_list.size() < copies()) // Try to return an extra one
        {
            if (pthread_rwlock_trywrlock(&m_values_lock) == 0)
            {
                auto ld = insert(tag, key);
                if (ld != nullptr)
                {
                    ld->inc();
                    return_list.push_back(ld);
                }
                pthread_rwlock_unlock(&m_values_lock);
            }
        }

        return return_list;
    }

  private:
    locked_dataset *insert(size_t tag, const uri_options_t &key)
    {
        auto current_time = ++m_time; // XXX
        int best_index = -1;
        atime_t best_atime = -1;

        for (size_t i = 0; i < capacity(); ++i)
        {
            if (m_atimes[i] < best_atime && m_values[i].lock_for_deletion())
            {
                if (best_index != -1)
                {
                    m_values[best_index].unlock();
                }
                best_index = i;
                best_atime = m_atimes[i];
            }
        }
        if (best_index != -1)
        {
            m_tags[best_index] = tag;
            m_atimes[best_index] = current_time;
            m_keys[best_index] = key;
            m_values[best_index] = std::move(locked_dataset(key));
            m_size++;
            return &(m_values[best_index]);
        }
        else
        {
            return nullptr;
        }
    }

  private:
    std::vector<size_t> m_tags;
    std::vector<atime_t> m_atimes;
    std::vector<key_t> m_keys;
    std::vector<value_t> m_values;
    size_t m_capacity;
    size_t m_copies;
    mutable pthread_rwlock_t m_tag_lock;
    mutable pthread_rwlock_t m_atimes_lock;
    mutable pthread_rwlock_t m_keys_lock;
    mutable pthread_rwlock_t m_values_lock;
    atime_t m_time;
    size_t m_size;
};

#endif // __CACHE_HPP__
