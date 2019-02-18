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

#ifndef __TOKENS_HPP__
#define __TOKENS_HPP__

#include <optional>
#include "types.hpp"

void token_init();
void token_deinit();
std::optional<uri_options_t> query_token(uint64_t token);

// The prototypes for get_token and surrender_token are in bindings.h

#endif
