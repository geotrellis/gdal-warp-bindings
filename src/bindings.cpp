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

#include <errno.h>
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

constexpr useconds_t MAX_US = (1 << 16);

cache_t *cache = nullptr;

/**
 * A macro for making one attempt to perform the given operation on
 * (one of) the locked datasets.  If an attempt succeeds, then the
 * variable `done` is set to `true`, otherwise it remains false.  In
 * either case, the reference count of each dataset is decremented.
 *
 * @param fn The operation to perform
 */
#define TRY(fn)                     \
    for (auto ld : locked_datasets) \
    {                               \
        if (!done && (ld->fn != 0)) \
        {                           \
            done = true;            \
        }                           \
        ld->dec();                  \
    }

/**
 * A macro for making some number of attempts to perform an operation
 * on (one of) a list of locked datasets.  If an attempt succeeds,
 * return the number of attempts made (so that intelligent tuning is
 * possible in the application that uses this library), otherwise
 * return the negative of some errno.
 *
 * @param fn The operation to perform
 */
#define DOIT(fn)                                                   \
    bool done = false;                                             \
    auto query_result = query_token(token);                        \
    if (query_result)                                              \
    {                                                              \
        auto uri_options = query_result.get();                     \
        int i;                                                     \
        for (i = 0; (i < attempts || attempts <= 0) && !done; ++i) \
        {                                                          \
            auto locked_datasets = cache->get(uri_options, -4);    \
            if (locked_datasets.size() == 0)                       \
            {                                                      \
                return -EAGAIN;                                    \
            }                                                      \
            TRY(fn)                                                \
            useconds_t us = 1 << i;                                \
            usleep(us <= MAX_US ? us : MAX_US);                    \
        }                                                          \
        if (i < attempts || (i > 0 && attempts == 0))              \
        {                                                          \
            return i;                                              \
        }                                                          \
        else                                                       \
        {                                                          \
            return -EAGAIN;                                        \
        }                                                          \
    }                                                              \
    else                                                           \
    {                                                              \
        return -ENOENT;                                            \
    }

/**
 * The initialization function for the library.
 *
 * @param size The maximum number of locked_dataset objects (each
 *             containing two GDAL Dataset objects) that can be live
 *             at one time.
 */
void init(size_t size)
{
    deinit();

    GDALAllRegister();

    cache = new cache_t(size);
    if (cache == nullptr)
    {
        throw std::bad_alloc();
    }

    token_init(640 * (1 << 10)); // This should be enough for anyone

    return;
}

/**
 * The deinitialization function for the library.
 */
void deinit()
{
    if (cache != nullptr)
    {
        delete cache;
        cache = nullptr;
    }
    token_deinit();
}

/**
 * Get the widths and heights of all overviews.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param widths An array of integers to receive the widths of the
 *               various overviews
 * @param heights An array of integers to receive the heights of the
 *                various overviews
 * @param max_length The maximum number of widths and heights to
 *                   return (nominally the smaller of the lengths of
 *                   the two arrays)
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_overview_widths_heights(uint64_t token, int dataset, int attempts, int *widths, int *heights, int max_length)
{
    DOIT(get_overview_widths_heights(dataset, widths, heights, max_length))
}

/**
 * Get the CRS in PROJ.4 format.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param crs The character array in-which to return the PROJ.4 string
 * @param max_size The size of the pre-allocated return buffer
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_crs_proj4(uint64_t token, int dataset, int attempts, char *crs, int max_size)
{
    DOIT(get_crs_proj4(dataset, crs, max_size));
}

/**
 * Get the CRS in WKT format.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param crs The character array in-which to return the PROJ.4 string
 * @param max_size The size of the pre-allocated return buffer
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_crs_wkt(uint64_t token, int dataset, int attempts, char *crs, int max_size)
{
    DOIT(get_crs_wkt(dataset, crs, max_size))
}

/**
 * Get the NODATA value for a band.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param band The band of interest
 * @param nodata The return-location of the NODATA value
 * @param success The return-location of the success flag (answer
 *                whether or not there is a NODATA value)
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_band_nodata(uint64_t token, int dataset, int attempts, int band, double *nodata, int *success)
{
    DOIT(get_band_nodata(dataset, band, nodata, success))
}

int get_band_min_max(uint64_t token, int dataset, int attempts, int band, int approx_okay, double *minmax, int *success)
{
    DOIT(get_band_max_min(dataset, band, approx_okay, minmax, success));
}

/**
 * Get the data type of a given band.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param band The band of interest
 * @param data_type The return-location of the band type (of integral
 *                  type GDALDataType)
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_band_data_type(uint64_t token, int dataset, int attempts, int band, int *data_type)
{
    auto ptr = reinterpret_cast<GDALDataType *>(data_type);
    DOIT(get_band_data_type(dataset, band, ptr));
}

/**
 * Get the number of bands.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param band_count The return-location of the band count
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_band_count(uint64_t token, int dataset, int attempts, int *band_count)
{
    DOIT(get_band_count(dataset, band_count))
}

/**
 * Get the width and height.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param width The return-location of the width
 * @param height The return-location of the height
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_width_height(uint64_t token, int dataset, int attempts, int *width, int *height)
{
    DOIT(get_width_height(dataset, width, height))
}

/**
 * Get pixel data.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param src_window See https://www.gdal.org/gdal_8h.html#aaffc6d9720dcb3c89ad0b88560bdf407
 * @param dst_window See https://www.gdal.org/gdal_8h.html#aaffc6d9720dcb3c89ad0b88560bdf407
 * @param band_number The band number of interest
 * @param _type The desired type of returned pixels (the argument is
 *              of integral type GDALDataType)
 * @param data The return-location of the read read data
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
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

/**
 * Get the the transform.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param transform The return location for the six double-precision
 *                  floating point number that will be returned
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_transform(uint64_t token, int dataset, int attempts, double transform[6])
{
    DOIT(get_transform(dataset, transform))
}
