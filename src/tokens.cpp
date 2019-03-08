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

#include <cstdint>
#include <ctime>
#include <exception>
#include <boost/optional.hpp>
#include <pthread.h>
#include "lru_cache.hpp"
#include "bindings.h"
#include "tokens.hpp"

static pthread_mutex_t token_lock;
static lru_cache<token_t, uri_options_t> *cache;

void token_init(size_t size)
{
    srand(time(nullptr));

    token_lock = PTHREAD_MUTEX_INITIALIZER;
    cache = new lru_cache<token_t, uri_options_t>(size);
}

void token_deinit()
{
    delete cache;
}

static token_t generate_token()
{
    auto upper_half = static_cast<uint64_t>(rand());
    auto lower_half = static_cast<uint64_t>(rand());
    auto retval = (upper_half << 32) | lower_half;

    return static_cast<token_t>(retval);
}

uint64_t get_token(const char *_uri, const char **_options)
{
    auto token = static_cast<token_t>(33);
    auto uri_options = std::make_pair(uri_t(_uri), options_t());

    // Construct uri_options object from uri and options
    while (*_options != nullptr)
    {
        uri_options.second.push_back(std::string(*_options));
        _options++;
    }

    pthread_mutex_lock(&token_lock);
    auto maybe_token = cache->get(uri_options);

    if (maybe_token) // uri ⨯ options pair already registered
    {
        pthread_mutex_unlock(&token_lock);
        return maybe_token.value();
    }
    else // uri ⨯ options pair not already registered
    {
        while (cache->contains(token))
        {
            token = generate_token();
        }
        cache->insert(token, uri_options);
        pthread_mutex_unlock(&token_lock);
    }

    return static_cast<uint64_t>(token);
}

boost::optional<uri_options_t> query_token(uint64_t _token)
{
    pthread_mutex_lock(&token_lock);
    auto token = static_cast<token_t>(_token);
    auto maybe_uri_options = cache->get(token);
    pthread_mutex_unlock(&token_lock);

    return maybe_uri_options;
}
