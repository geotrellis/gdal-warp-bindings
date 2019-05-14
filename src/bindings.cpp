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
#if defined(__linux__) || defined(__APPLE__)
#include <csignal>
#endif
#include <ctime>
#include <exception>
#include <string>
#include <map>
#include <vector>

#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <gdal.h>

#include "bindings.h"
#include "types.hpp"
#include "flat_lru_cache.hpp"
#include "locked_dataset.hpp"
#include "tokens.hpp"

typedef flat_lru_cache cache_t;

pthread_mutex_t livelock_mutex = PTHREAD_MUTEX_INITIALIZER;

cache_t *cache = nullptr;

#if defined(__linux__) || defined(__APPLE__)
struct sigaction sa_old, sa_new;
bool handler_installed = false;

// SIGTERM handler
static void sigterm_handler(int signal)
{
    if (signal == SIGTERM)
    {
        raise(SIGTRAP);
        return;
    }
}
#endif

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
        if (!done)                  \
        {                           \
            ++touched;              \
            if (ld->fn != false)    \
            {                       \
                done = true;        \
            }                       \
        }                           \
        ld->dec();                  \
    }

#define TOO_MANY_ITERATIONS (attempts > (1 << 5) ? attempts - 33 : 1 << 16)

/**
 * A macro for making some number of attempts to perform an operation
 * on (one of) a list of locked datasets.  If an attempt succeeds,
 * return the number of attempts made (so that intelligent tuning is
 * possible in the application that uses this library), otherwise
 * return the negative of some errno.
 *
 * @param fn The operation to perform
 */
#define DOIT(fn)                                                    \
    bool done = false;                                              \
    auto query_result = query_token(token);                         \
    if (query_result)                                               \
    {                                                               \
        auto uri_options = query_result.get();                      \
        bool has_lock = false;                                      \
        int touched = 0;                                            \
        int i;                                                      \
        for (i = 0; (i < attempts || attempts <= 0) && !done; ++i)  \
        {                                                           \
            if (i >= TOO_MANY_ITERATIONS && !has_lock)              \
            {                                                       \
                pthread_mutex_lock(&livelock_mutex);                \
                has_lock = true;                                    \
            }                                                       \
            auto locked_datasets = cache->get(uri_options, copies); \
            const auto num_datasets = locked_datasets.size();       \
            if (num_datasets == 0)                                  \
            {                                                       \
                if (has_lock)                                       \
                {                                                   \
                    pthread_mutex_unlock(&livelock_mutex);          \
                }                                                   \
                return -EAGAIN;                                     \
            }                                                       \
            TRY(fn)                                                 \
            if (!done && !has_lock)                                 \
            {                                                       \
                sleep(0);                                           \
            }                                                       \
        }                                                           \
        if (has_lock)                                               \
        {                                                           \
            pthread_mutex_unlock(&livelock_mutex);                  \
        }                                                           \
        if (i < attempts || (i > 0 && attempts == 0))               \
        {                                                           \
            return touched;                                         \
        }                                                           \
        else                                                        \
        {                                                           \
            return -EAGAIN;                                         \
        }                                                           \
    }                                                               \
    else                                                            \
    {                                                               \
        return -ENOENT;                                             \
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

#if defined(__linux__) || defined(__APPLE__)
    if (getenv("GDALWARP_SIGTERM_DUMP") != NULL)
    {
        sa_new.sa_handler = sigterm_handler;
        handler_installed = true;

        if (sigaction(SIGTERM, &sa_new, &sa_old) == -1)
        {
            fprintf(stderr, "Unable to install SIGTERM handler\n");
            exit(-1);
        }
    }
#endif

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

#if defined(__linux__) || defined(__APPLE__)
    if (handler_installed)
    {
        sigaction(SIGTERM, &sa_old, nullptr);
    }
#endif

    token_deinit();
}

#if defined(SO_FINI) && defined(__linux__)
void __attribute__((destructor)) fini(void)
{
    deinit();
    GDALDestroy();
}
#endif

/**
 * Get the block size of the given band.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_number The band in question
 * @param width The return-location of the block width
 * @param height The return-location of the block height
 * @return True iff the operation succeeded
 */
int get_block_size(uint64_t token, int dataset, int attempts, int copies,
                   int band_number, int *width, int *height)
{
    DOIT(get_block_size(dataset, band_number, width, height));
}

/**
 * Get the offset of the given band.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_number The band in question
 * @param offset The return-location of the offset
 * @param success The return-location of the success flag
 * @return True iff the operation succeeded
 */
int get_offset(uint64_t token, int dataset, int attempts, int copies,
               int band_number, double *offset, int *success)
{
    DOIT(get_offset(dataset, band_number, offset, success));
}

/**
 * Get the scale of the given band.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_number The band in question
 * @param scale The return-location of the scale
 * @param success The return-location of the success flag
 * @return True iff the operation succeeded
 */
int get_scale(uint64_t token, int dataset, int attempts, int copies,
              int band_number, double *scale, int *success)
{
    DOIT(get_scale(dataset, band_number, scale, success));
}

/**
 * Get the color interpretation of the given band.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_number The band in question
 * @param color_interp The return-slot for the integer-coded color
 *                     interpretation
 * @return True iff the operation succeeded
 */
int get_color_interpretation(uint64_t token, int dataset, int attempts, int copies,
                             int band_number, int *color_interp)
{
    DOIT(get_color_interpretation(dataset, band_number, color_interp));
}

/**
 * Get the list of metadata domain lists.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_number The band to query (zero for the file itself)
 * @param domain_list The return-location for the list of strings
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_metadata_domain_list(uint64_t token, int dataset, int attempts, int copies,
                             int band_number, char ***domain_list)
{
    DOIT(get_metadata_domain_list(dataset, band_number, domain_list));
}

/**
 * Get the metadata found in a particular metadata domain.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_number The band to query (zero for the file itself)
 * @param domain The metadata domain to query
 * @param list The return-location for the list of strings
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_metadata(uint64_t token, int dataset, int attempts, int copies,
                 int band_number, const char *domain, char ***list)
{
    DOIT(get_metadata(dataset, band_number, domain, list));
}

/**
 * Get a particular metadata value associated with a key.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_number The band to query (zero for the file itself)
 * @param key The key of the key ⨯ value metadata pair
 * @param domain The metadata domain to query
 * @param value The return-location for the value of the key ⨯ value pair
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_metadata_item(uint64_t token, int dataset, int attempts, int copies,
                      int band_number, const char *key, const char *domain, const char **value)
{
    DOIT(get_metadata_item(dataset, band_number, key, domain, value));
}

/**
 * Get the widths and heights of all overviews.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_number The band of interest
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
int get_overview_widths_heights(uint64_t token, int dataset, int attempts, int copies,
                                int band_number, int *widths, int *heights, int max_length)
{
    DOIT(get_overview_widths_heights(dataset, band_number, widths, heights, max_length))
}

/**
 * Get the CRS in PROJ.4 format.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param crs The character array in-which to return the PROJ.4 string
 * @param max_size The size of the pre-allocated return buffer
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_crs_proj4(uint64_t token, int dataset, int attempts, int copies,
                  char *crs, int max_size)
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
 * @param copies The desired number of datasets
 * @param attempts The number of attempts to make before giving up
 * @param crs The character array in-which to return the PROJ.4 string
 * @param max_size The size of the pre-allocated return buffer
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_crs_wkt(uint64_t token, int dataset, int attempts, int copies,
                char *crs, int max_size)
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
 * @param copies The desired number of datasets
 * @param band_number The band of interest
 * @param nodata The return-location of the NODATA value
 * @param success The return-location of the success flag (answer
 *                whether or not there is a NODATA value)
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_band_nodata(uint64_t token, int dataset, int attempts, int copies,
                    int band_number, double *nodata, int *success)
{
    DOIT(get_band_nodata(dataset, band_number, nodata, success))
}

/**
 * Get the minimum and maximum values found in the band.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_number The band_number of interest
 * @param approx_okay Is it okay to approximate the value if it is not
 *                    stored in the metadata, or must it be calculated
 *                    exactly?  An integer treated as a boolean
 * @param minmax The return-location of the minimum and maximum
 * @param success The return-location of the success flag (answer
 *                whether or not there is a NODATA value)
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_band_min_max(uint64_t token, int dataset, int attempts, int copies,
                     int band_number, int approx_okay, double *minmax, int *success)
{
    DOIT(get_band_max_min(dataset, band_number, approx_okay, minmax, success));
}

/**
 * Get the data type of a given band.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_number The band of interest
 * @param data_type The return-location of the band_number type (of integral
 *                  type GDALDataType)
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_band_data_type(uint64_t token, int dataset, int attempts, int copies,
                       int band_number, int *data_type)
{
    auto ptr = reinterpret_cast<GDALDataType *>(data_type);
    DOIT(get_band_data_type(dataset, band_number, ptr));
}

/**
 * Get the number of bands.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_count The return-location of the band count
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_band_count(uint64_t token, int dataset, int attempts, int copies,
                   int *band_count)
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
 * @param copies The desired number of datasets
 * @param width The return-location of the width
 * @param height The return-location of the height
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_width_height(uint64_t token, int dataset, int attempts, int copies,
                     int *width, int *height)
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
 * @param copies The desired number of datasets
 * @param src_window See https://www.gdal.org/gdal_8h.html#aaffc6d9720dcb3c89ad0b88560bdf407
 * @param dst_window See https://www.gdal.org/gdal_8h.html#aaffc6d9720dcb3c89ad0b88560bdf407
 * @param band_number The band_number number of interest
 * @param _type The desired type of returned pixels (the argument is
 *              of integral type GDALDataType)
 * @param data The return-location of the read read data
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_data(uint64_t token, int dataset, int attempts, int copies,
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
 * @param copies The desired number of datasets
 * @param transform The return location for the six double-precision
 *                  floating point number that will be returned
 * @return The number of attempts made (upon success) or a negative
 *         errno (upon failure)
 */
int get_transform(uint64_t token, int dataset, int attempts, int copies,
                  double transform[6])
{
    DOIT(get_transform(dataset, transform))
}
