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

#include <exception>
#include "bindings.h"
#include "tokens.hpp"

token_map_t *token_map = nullptr;
reverse_token_map_t *reverse_token_map = nullptr;
pthread_mutex_t token_lock;

void token_init()
{
    srand(time(nullptr));

    token_lock = PTHREAD_MUTEX_INITIALIZER;
    token_map = new token_map_t();
    reverse_token_map = new reverse_token_map_t();

    if (reverse_token_map == nullptr || token_map == nullptr)
    {
        throw std::bad_alloc();
    }
}
void token_deinit()
{
    if (reverse_token_map != nullptr)
    {
        delete reverse_token_map;
        reverse_token_map = nullptr;
    }
    if (token_map != nullptr)

    {
        delete token_map;
        token_map = nullptr;
    }
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

    while (*_options != nullptr)
    {
        uri_options.second.push_back(std::string(*_options));
        _options++;
    }

    pthread_mutex_lock(&token_lock);
    auto itr = token_map->find(uri_options);
    if (itr != token_map->end())
    {
        auto retval = itr->second;
        pthread_mutex_unlock(&token_lock);
        return retval;
    }
    while (reverse_token_map->count(token) > 0)
    {
        token = generate_token();
    }
    token_map->insert(std::make_pair(uri_options, token));
    reverse_token_map->insert(std::make_pair(token, uri_options));
    pthread_mutex_unlock(&token_lock);

    return static_cast<uint64_t>(token);
}

void surrender_token(uint64_t _token)
{
    pthread_mutex_lock(&token_lock);
    auto token = static_cast<token_t>(_token);
    auto itr = reverse_token_map->find(token);

    if (itr != reverse_token_map->end())
    {
        const auto &uri_options = itr->second;
        reverse_token_map->erase(token);
        token_map->erase(uri_options);
    }
    pthread_mutex_unlock(&token_lock);
}
