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

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <exception>
#include <string>
#include <map>
#include <vector>

#include <pthread.h>
#include <gdal.h>

#include "bindings.h"
#include "types.hpp"
#include "lru_cache.hpp"
#include "locked_dataset.hpp"
#include "tokens.hpp"

typedef lru_cache cache_t;

cache_t *cache = nullptr;

void init(int size)
{
    deinit();

    GDALAllRegister();

    cache = new cache_t(size);

    if (cache == nullptr)
    {
        throw std::bad_alloc();
    }

    return;
}

void deinit()
{
    if (cache != nullptr)
    {
        delete cache;
    }
}

int read_data(uint64_t token,
              double src_window[4],
              int dst_window[2],
              int band_number,
              GDALDataType type,
              void *data)
{
    return 33;
}
