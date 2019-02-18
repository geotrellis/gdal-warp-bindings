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

#ifndef __TYPES_H__
#define __TYPES_H__

#include <map>
#include <string>
#include <vector>

typedef std::vector<std::string> options_t;
typedef std::string uri_t;
typedef std::pair<uri_t, options_t> uri_options_t;
typedef uint64_t token_t;
typedef std::map<uri_options_t, token_t> token_map_t;
typedef std::map<token_t, uri_options_t> reverse_token_map_t;

#endif
