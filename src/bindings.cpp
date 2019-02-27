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

#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include <gdal.h>

#include "bindings.h"
#include "types.hpp"
#include "cache.hpp"
#include "locked_dataset.hpp"
#include "tokens.hpp"

typedef lru_cache cache_t;

cache_t *cache = nullptr;

#define TRY(fn)                     \
    for (auto ld : locked_datasets) \
    {                               \
        if (!done && ld->fn)        \
        {                           \
            done = true;            \
        }                           \
        ld->dec();                  \
    }

#define DOIT(fn)                                                       \
    bool done = false;                                                 \
    auto query_result = query_token(token);                            \
    if (query_result.has_value())                                      \
    {                                                                  \
        auto uri_options = query_result.value();                       \
        for (int i = 0; (i < attempts || attempts <= 0) && !done; ++i) \
        {                                                              \
            auto locked_datasets = cache->get(uri_options);            \
            TRY(fn)                                                    \
            sched_yield();                                             \
        }                                                              \
    }                                                                  \
    return done;

void init(int size, int copies)
{
    deinit();

    GDALAllRegister();

    cache = new cache_t(size, copies);
    if (cache == nullptr)
    {
        throw std::bad_alloc();
    }

    token_init();

    return;
}

void deinit()
{
    if (cache != nullptr)
    {
        fprintf(stderr, "%ld\n", cache->size());
        delete cache;
    }
    token_deinit();
}

int get_band_nodata(uint64_t token, int attempts, int band, double *nodata, int *success)
{
    DOIT(get_band_nodata(band, nodata, success))
}

int get_band_data_type(uint64_t token, int attempts, int band, int *data_type)
{
    auto p = reinterpret_cast<GDALDataType *>(data_type);
    DOIT(get_band_data_type(band, p));
}

int get_band_count(uint64_t token, int attempts, int *band_count)
{
    DOIT(get_band_count(band_count))
}

int get_width_height(uint64_t token, int attempts, int *width, int *height)
{
    DOIT(get_width_height(width, height))
}

int get_data(uint64_t token,
             int attempts,
             int src_window[4],
             int dst_window[2],
             int band_number,
             int _type,
             void *data)
{
    auto type = static_cast<GDALDataType>(_type);
    DOIT(get_pixels(src_window, dst_window, band_number, type, data))
}

int get_transform(uint64_t token, int attempts, double transform[6])
{
    DOIT(get_transform(transform))
}
