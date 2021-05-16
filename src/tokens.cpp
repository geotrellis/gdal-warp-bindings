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

#include <algorithm>
#include <cstdint>
#include <exception>
#include <random>

#include <boost/optional.hpp>
#include <boost/compute/detail/lru_cache.hpp>

#include <pthread.h>

#include "bindings.h"
#include "tokens.hpp"

typedef boost::compute::detail::lru_cache<token_t, uri_options_t> lru_cache;
static pthread_mutex_t token_lock;
static lru_cache *cache = nullptr;
static std::mt19937_64 g;
static std::uniform_int_distribution<token_t> dist;

/**
 * Initialize the token-management part of the library.
 */
void token_init(size_t size)
{
    g = std::mt19937_64(std::random_device{}());
#if defined(_GNU_SOURCE)
    token_lock = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
#else
    token_lock = PTHREAD_MUTEX_INITIALIZER;
#endif
    cache = new lru_cache(size);
}

/**
 * Deinitialize the token-management part of the library.
 */
void token_deinit()
{
    if (cache != nullptr)
    {
        delete cache;
    }
    cache = nullptr;
}

/**
 * Generate a token.
 *
 * @return A (possibly used, possibly unused) token
 */
static token_t generate_token()
{
    return dist(g);
}

/**
 * Return an unused token for the given uri ⨯ options pair.  This is
 * not a function: two subsequent calls to it with the same pair as
 * input may produce different output.
 *
 * @param _uri A C-style string containing the URI
 * @param _options An array of C-style strings contains the warp options
 * @return A token that was not in use prior to the call
 */
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

    if (pthread_mutex_lock(&token_lock) != 0)
    {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        return BAD_TOKEN;
    }
    while (cache->contains(token) || token == BAD_TOKEN)
    {
        token = generate_token();
    }
    cache->insert(token, uri_options);
    pthread_mutex_unlock(&token_lock);

    return static_cast<uint64_t>(token);
}

/**
 * Get the uri ⨯ options pair associated with a token (if one exists).
 *
 * @param token The token of interest
 * @return An optional of type uri_options_t
 */
boost::optional<uri_options_t> query_token(uint64_t _token)
{
    if (_token == BAD_TOKEN)
    {
        return boost::optional<uri_options_t>();
    }
    else if (pthread_mutex_lock(&token_lock) != 0)
    {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        return boost::optional<uri_options_t>();
    }
    else
    {
        auto token = static_cast<token_t>(_token);
        auto maybe_uri_options = cache->get(token);
        pthread_mutex_unlock(&token_lock);

        return maybe_uri_options;
    }
}
