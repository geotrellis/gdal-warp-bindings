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
#include "errorcodes.hpp"

static uint64_t default_nanos = 0;

typedef flat_lru_cache cache_t;
static cache_t *cache = nullptr;

#if defined(__linux__) || defined(__APPLE__)
static struct sigaction sa_old, sa_new;
static bool handler_installed = false;

// SIGTERM handler
static void sigterm_handler(int signal)
{
    if (signal == SIGTERM)
    {
        raise(SIGSEGV);
        return;
    }
}
#endif

static uint64_t get_nanos()
{
#if defined(__linux__)
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (((uint64_t)ts.tv_sec) * 1000000000) + ts.tv_nsec;
#else
    return 0;
#endif
}

#if defined(__APPLE__)
inline void pthread_yield()
{
    pthread_yield_np();
}
#elif defined(__MINGW32__)
inline void pthread_yield()
{
    sleep(0);
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
#define TRY(fn)                                                       \
    for (auto ld : locked_datasets)                                   \
    {                                                                 \
        if (!done)                                                    \
        {                                                             \
            ++touched;                                                \
            code = ld->fn;                                            \
            if (code == ATTEMPT_SUCCESSFUL && code != DATASET_LOCKED) \
            {                                                         \
                done = true;                                          \
            }                                                         \
        }                                                             \
        ld->dec();                                                    \
    }

/**
 * A macro for making some number of attempts to perform an operation
 * on (one of) a list of locked datasets.  If an attempt succeeds,
 * return the number of attempts made (so that intelligent tuning is
 * possible in the application that uses this library), otherwise
 * return the negative of some CPLErrorNum (see
 * https://gdal.org/doxygen/cpl__error_8h.html).
 *
 * @param fn The operation to perform
 */
#define DOIT(fn)                                                                          \
    bool done = false;                                                                    \
    auto query_result = query_token(token);                                               \
    int code = -CPLE_AppDefined;                                                          \
    uint64_t then, now;                                                                   \
    if (query_result)                                                                     \
    {                                                                                     \
        auto uri_options = query_result.get();                                            \
        then = get_nanos();                                                               \
        int touched = 0;                                                                  \
        int i;                                                                            \
        for (i = 0; (i < attempts || attempts <= 0) && !done; ++i)                        \
        {                                                                                 \
            now = get_nanos();                                                            \
            if ((nanos > 0) && (now - then > nanos))                                      \
            {                                                                             \
                return -CPLE_FileIO;                                                      \
            }                                                                             \
            auto locked_datasets = cache->get(uri_options, copies);                       \
            const auto num_datasets = locked_datasets.size();                             \
            if (num_datasets == 0)                                                        \
            {                                                                             \
                return -CPLE_OpenFailed;                                                  \
            }                                                                             \
            TRY(fn)                                                                       \
            if (!done)                                                                    \
            {                                                                             \
                pthread_yield();                                                          \
            }                                                                             \
        }                                                                                 \
        if ((code == ATTEMPT_SUCCESSFUL) && ((i < attempts) || (i > 0 && attempts == 0))) \
        {                                                                                 \
            return touched;                                                               \
        }                                                                                 \
        else if (code == ATTEMPT_SUCCESSFUL || code == DATASET_LOCKED)                    \
        {                                                                                 \
            return -CPLE_FileIO;                                                          \
        }                                                                                 \
        else                                                                              \
        {                                                                                 \
            return code;                                                                  \
        }                                                                                 \
    }                                                                                     \
    else                                                                                  \
    {                                                                                     \
        return -CPLE_OpenFailed;                                                          \
    }

/**
 * Initialize various globals from values in the environment and
 * (possibly) install the SIGTERM signal handler.
 *
 * @param size A pointer to the desired maximum number of datasets
 */
void env_init(size_t *size)
{
    const char *env_ptr = nullptr;

#if defined(__linux__)
    env_ptr = getenv("GDALWARP_DEFAULT_NANOS");
    if (env_ptr != nullptr)
    {
        sscanf(env_ptr, "%lud", &default_nanos);
    }
#else
    default_nanos = 0;
#endif

    env_ptr = getenv("GDALWARP_NUM_DATASETS");
    if (env_ptr != nullptr)
    {
#if defined(__MINGW32__)
        sscanf(env_ptr, "%lld", size);
#else
        sscanf(env_ptr, "%ld", size);
#endif
    }

#if defined(__linux__) || defined(__APPLE__)
    if (getenv("GDALWARP_SIGTERM_DUMP") != nullptr)
    {
        sa_new.sa_handler = sigterm_handler;
        handler_installed = true;

        if (default_nanos == 0)
        {
            default_nanos = 250000000;
        }

        if (sigaction(SIGTERM, &sa_new, &sa_old) == -1)
        {
            fprintf(stderr, "Unable to install SIGTERM handler\n");
            exit(-1);
        }
    }
#endif
}

/**
 * Deinitialize environmentally-controlled structures and behaviors.
 */
void env_deinit()
{
#if defined(__linux__) || defined(__APPLE__)
    if (handler_installed)
    {
        sigaction(SIGTERM, &sa_old, nullptr);
    }
#endif
}

/**
 * Initialize the dataset cache.
 *
 * @param size The maximum number of entries in the cache
 */
void cache_init(size_t size)
{
    cache = new cache_t{size};
    if (cache == nullptr)
    {
        throw std::bad_alloc();
    }
}

/**
 * Deinitialize the dataset cache.
 */
void cache_deinit()
{
    if (cache != nullptr)
    {
        delete cache;
        cache = nullptr;
    }
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
    errno_init();
    env_init(&size);
    cache_init(size);
    token_init(640 * (1 << 10));

    return;
}

/**
 * The deinitialization function for the library.
 */
void deinit()
{
    errno_deinit();
    env_deinit();
    cache_deinit();
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
 * Noop.
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int noop(uint64_t token, int dataset, int attempts, int copies)
{
    uint64_t nanos = default_nanos;
    DOIT(noop());
}

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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_block_size(uint64_t token, int dataset, int attempts, int copies,
                   int band_number, int *width, int *height)
{
    uint64_t nanos = default_nanos;
    DOIT(get_block_size(dataset, band_number, width, height));
}

/** Get the histogram of a given band
 *
 * @param token A token associated with some uri ⨯ options pair
 * @param dataset 0 (or locked_dataset::SOURCE) for the source
 *                dataset, 1 (or locked_dataset::WARPED) for the
 *                warped dataset
 * @param attempts The number of attempts to make before giving up
 * @param copies The desired number of datasets
 * @param band_number The band in question
 * @param lower the lower bound of the histogram
 * @param upper the upper bound of the histogram
 * @param num_buckets The number of histogram buckts
 * @param hist array into which the histogram totals are placed
 * @param include_out_of_range Whether to map out of range values into the first/last buckets
 * @param approx_ok Whether to accept an approximate histogram. With COGs, will cause the use of overviews
 */
int get_histogram(uint64_t token, int dataset, int attempts, int copies,
                  int band_number, double lower, double upper, int num_buckets,
                  GUIntBig *hist, int include_out_of_range, int approx_ok)
{
    uint64_t nanos = default_nanos;
    DOIT(get_histogram(dataset, band_number,
                       lower, upper, num_buckets, hist, include_out_of_range, approx_ok));
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_offset(uint64_t token, int dataset, int attempts, int copies,
               int band_number, double *offset, int *success)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_scale(uint64_t token, int dataset, int attempts, int copies,
              int band_number, double *scale, int *success)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_color_interpretation(uint64_t token, int dataset, int attempts, int copies,
                             int band_number, int *color_interp)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_metadata_domain_list(uint64_t token, int dataset, int attempts, int copies,
                             int band_number, char ***domain_list)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_metadata(uint64_t token, int dataset, int attempts, int copies,
                 int band_number, const char *domain, char ***list)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_metadata_item(uint64_t token, int dataset, int attempts, int copies,
                      int band_number, const char *key, const char *domain, const char **value)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_overview_widths_heights(uint64_t token, int dataset, int attempts, int copies,
                                int band_number, int *widths, int *heights, int max_length)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_crs_proj4(uint64_t token, int dataset, int attempts, int copies,
                  char *crs, int max_size)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_crs_wkt(uint64_t token, int dataset, int attempts, int copies,
                char *crs, int max_size)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_band_nodata(uint64_t token, int dataset, int attempts, int copies,
                    int band_number, double *nodata, int *success)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_band_min_max(uint64_t token, int dataset, int attempts, int copies,
                     int band_number, int approx_okay, double *minmax, int *success)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_band_data_type(uint64_t token, int dataset, int attempts, int copies,
                       int band_number, int *data_type)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_band_count(uint64_t token, int dataset, int attempts, int copies,
                   int *band_count)
{
    uint64_t nanos = default_nanos;
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_width_height(uint64_t token, int dataset, int attempts, int copies,
                     int *width, int *height)
{
    uint64_t nanos = default_nanos;
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
 * @param nanos The approximate time budget for this call (in nanoseconds)
 * @param copies The desired number of datasets
 * @param src_window Please see https://gdal.org/api/raster_c_api.html?highlight=rasterio#_CPPv419GDALDatasetRasterIO12GDALDatasetH10GDALRWFlagiiiiPvii12GDALDataTypeiPiiii
 * @param dst_window Please see https://gdal.org/api/raster_c_api.html?highlight=rasterio#_CPPv419GDALDatasetRasterIO12GDALDatasetH10GDALRWFlagiiiiPvii12GDALDataTypeiPiiii
 * @param band_number The band_number number of interest
 * @param _type The desired type of returned pixels (the argument is
 *              of integral type GDALDataType)
 * @param data The return-location of the read read data
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_data(uint64_t token, int dataset, int attempts, uint64_t nanos, int copies,
             int src_window[4],
             int dst_window[2],
             int band_number,
             int _type,
             void *data)
{
    auto type = static_cast<GDALDataType>(_type);
#if !defined(__linux__)
    nanos = 0;
#endif
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
 * @return The number of attempts on success, negative CPLErrorNum on failure
 */
int get_transform(uint64_t token, int dataset, int attempts, int copies,
                  double transform[6])
{
    uint64_t nanos = default_nanos;
    DOIT(get_transform(dataset, transform))
}
