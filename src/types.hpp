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

#ifndef __TYPES_H__
#define __TYPES_H__

#include <map>
#include <string>
#include <vector>
#include <functional>

typedef std::vector<std::string> options_t;
typedef std::string uri_t;
typedef std::pair<uri_t, options_t> uri_options_t;
typedef uint64_t token_t;
typedef std::map<uri_options_t, token_t> token_map_t;
typedef std::map<token_t, uri_options_t> reverse_token_map_t;

typedef std::hash<std::string> string_hash_t;
typedef std::hash<options_t> options_hash_t;
typedef std::hash<uri_options_t> uri_options_hash_t;

namespace std
{

/**
 * Hash function for options_t.
 */
template <>
struct hash<options_t>
{
    size_t operator()(const options_t &rhs) const
    {
        auto h = string_hash_t();
        size_t result = 0;

        for (auto str : rhs)
        {
            result += h(str);
        }
        return result;
    }
};

/**
 * Hash function for uri_options_t.
 */
template <>
struct hash<uri_options_t>
{
    size_t operator()(const uri_options_t &rhs) const
    {
        auto h1 = string_hash_t();
        auto h2 = options_hash_t();

        return h1(rhs.first) + h2(rhs.second);
    }
};
} // namespace std

#endif
