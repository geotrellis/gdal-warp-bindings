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
#include "flat_lru_cache.hpp"
#include "locked_dataset.hpp"
#include "tokens.hpp"

typedef flat_lru_cache cache_t;

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
    if (query_result)                                                  \
    {                                                                  \
        auto uri_options = query_result.get();                         \
        for (int i = 0; (i < attempts || attempts <= 0) && !done; ++i) \
        {                                                              \
            auto locked_datasets = cache->get(uri_options);            \
            TRY(fn)                                                    \
            sched_yield();                                             \
        }                                                              \
    }                                                                  \
    return done;

void init(size_t size, size_t copies)
{
    deinit();

    GDALAllRegister();

    cache = new cache_t(size, copies);
    if (cache == nullptr)
    {
        throw std::bad_alloc();
    }

    token_init(size << 1);

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

int get_overview_widths_heights(uint64_t token, int dataset, int attempts, int *widths, int *heights, int max_length)
{
    DOIT(get_overview_widths_heights(dataset, widths, heights, max_length))
}

int get_crs_proj4(uint64_t token, int dataset, int attempts, char *crs, int max_size)
{
    DOIT(get_crs_proj4(dataset, crs, max_size));
}

int get_crs_wkt(uint64_t token, int dataset, int attempts, char *crs, int max_size)
{
    DOIT(get_crs_wkt(dataset, crs, max_size))
}

int get_band_nodata(uint64_t token, int dataset, int attempts, int band, double *nodata, int *success)
{
    DOIT(get_band_nodata(dataset, band, nodata, success))
}

int get_band_data_type(uint64_t token, int dataset, int attempts, int band, int *data_type)
{
    auto ptr = reinterpret_cast<GDALDataType *>(data_type);
    DOIT(get_band_data_type(dataset, band, ptr));
}

int get_band_count(uint64_t token, int dataset, int attempts, int *band_count)
{
    DOIT(get_band_count(dataset, band_count))
}

int get_width_height(uint64_t token, int dataset, int attempts, int *width, int *height)
{
    DOIT(get_width_height(dataset, width, height))
}

int get_data(uint64_t token,
             int dataset,
             int attempts,
             int src_window[4],
             int dst_window[2],
             int band_number,
             int _type,
             void *data)
{
    auto type = static_cast<GDALDataType>(_type);
    DOIT(get_pixels(dataset, src_window, dst_window, band_number, type, data))
}

int get_transform(uint64_t token, int dataset, int attempts, double transform[6])
{
    DOIT(get_transform(dataset, transform))
}
