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

#ifndef __locked_dataset_H__
#define __locked_dataset_H__

#include <functional>

#if defined(__MINGW32__)
typedef _locale_t locale_t;
#endif

#include <cassert>
#include <cstring>

#include <atomic>
#include <limits>

#include <pthread.h>

#include <gdal.h>
#include <gdal_utils.h>
#include <cpl_conv.h>
#include <ogr_srs_api.h>

#include "types.hpp"
#include "errorcodes.hpp"

typedef std::atomic<int> atomic_int_t;

constexpr int ATTEMPT_SUCCESSFUL = std::numeric_limits<int>::max();
constexpr int DATASET_LOCKED = std::numeric_limits<int>::lowest();

#define TRYLOCK                                      \
    if (pthread_mutex_trylock(&m_dataset_lock) != 0) \
    {                                                \
        return DATASET_LOCKED;                       \
    }

#define SUCCESS return ATTEMPT_SUCCESSFUL;
#define FAILURE                        \
    {                                  \
        int retval = get_last_errno(); \
        if (retval == CE_None)         \
        {                              \
            return -CPLE_ObjectNull;   \
        }                              \
        else                           \
        {                              \
            return -retval;            \
        }                              \
    }
#define UNLOCK pthread_mutex_unlock(&m_dataset_lock);

class locked_dataset
{
public:
    locked_dataset()
        : m_datasets{nullptr, nullptr},
          m_uri_options(),
#if defined(_GNU_SOURCE)
          m_dataset_lock(PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP),
#else
          m_dataset_lock(PTHREAD_MUTEX_INITIALIZER),
#endif
          m_use_count(0)
    {
    }

    locked_dataset(const uri_options_t &uri_options)
        : m_datasets{nullptr, nullptr},
          m_uri_options(uri_options),
#if defined(_GNU_SOURCE)
          m_dataset_lock(PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP),
#else
          m_dataset_lock(PTHREAD_MUTEX_INITIALIZER),
#endif
          m_use_count(0)
    {
        open();
    }

    locked_dataset(locked_dataset &rhs) = delete;

    locked_dataset(locked_dataset &&rhs) noexcept
        : m_uri_options(std::move(rhs.m_uri_options)),
#if defined(_GNU_SOURCE)
          m_dataset_lock(PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP),
#else
          m_dataset_lock(PTHREAD_MUTEX_INITIALIZER),
#endif
          m_use_count(0) // rhs known to be zero
    {
        assert(rhs.m_use_count == 0);

        // XXX not thread safe, but the rhs should always be a
        // just-created local that is not in use anywhere else.
        m_datasets[SOURCE] = std::exchange(rhs.m_datasets[SOURCE], nullptr);
        m_datasets[WARPED] = std::exchange(rhs.m_datasets[WARPED], nullptr);
    }

    locked_dataset &operator=(locked_dataset &rhs) = delete;

    locked_dataset &operator=(locked_dataset &&rhs) noexcept
    {
        assert(m_use_count == 0);
        assert(rhs.m_use_count == 0);

        // XXX This does not look thread safe, but this is only called
        // when either (a) the lhs has been locked or (b) the lhs is
        // not in use (as when the `clear` method on the cache is
        // called during initialization) and the rhs is a just-created
        // dataset that is not in use anywhere else.
        close();

        m_datasets[SOURCE] = std::exchange(rhs.m_datasets[SOURCE], nullptr);
        m_datasets[WARPED] = std::exchange(rhs.m_datasets[WARPED], nullptr);
        m_uri_options = std::move(rhs.m_uri_options);

        // m_dataset_lock known to be locked prior to this call if
        // this is a valid dataset
        UNLOCK

        return *this;
    }

    bool operator==(const uri_options_t &rhs) const
    {
        return (m_uri_options == rhs);
    }

    ~locked_dataset()
    {
        close();
    }

    /**
     * Noop.
     *
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int noop()
    {
        TRYLOCK
        UNLOCK
        SUCCESS
    }

    /**
     * Get the block size of the given band.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band in question
     * @param width The return-location of the block width
     * @param height The return-location of the block height
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_block_size(int dataset, int band_number, int *width, int *height)
    {
        TRYLOCK
        GDALRasterBandH band = GDALGetRasterBand(m_datasets[dataset], band_number);
        GDALGetBlockSize(band, width, height);
        UNLOCK
        SUCCESS
    }

    /** Get a histogram of a band
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band in question
     * @param lower the lower bound of the histogram
     * @param upper the upper bound of the histogram
     * @param num_buckets The number of histogram buckts
     * @param hist array into which the histogram totals are placed
     * @param include_out_of_range Whether to map out of range values into the first/last buckets
     * @param approx_ok Whether to accept an approximate histogram. With COGs, will cause the use of overviews
     */
    int get_histogram(int dataset, int band_number,
                      double lower, double upper,
                      int num_buckets, GUIntBig *hist, int include_out_of_range, int approx_ok)
    {
        TRYLOCK
        GDALRasterBandH bandh = GDALGetRasterBand(m_datasets[dataset], band_number);
        auto retval = GDALGetRasterHistogramEx(bandh, lower, upper, num_buckets,
                                               hist, include_out_of_range, approx_ok, NULL, NULL);
        UNLOCK
        if (retval == CE_None)
        {
            SUCCESS
        }
        else
        {
            FAILURE
        }
    }

    /**
     * Get the offset of the given band.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band in question
     * @param offset The return-location of the offset
     * @param success The return-location of the success flag
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_offset(int dataset, int band_number, double *offset, int *success)
    {
        TRYLOCK
        GDALRasterBandH bandh = GDALGetRasterBand(m_datasets[dataset], band_number);
        *offset = GDALGetRasterOffset(bandh, success);
        UNLOCK
        SUCCESS
    }

    /**
     * Get the scale of the given band.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band in question
     * @param scale The return-location of the scale
     * @param success The return-location of the success flag
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_scale(int dataset, int band_number, double *scale, int *success)
    {
        TRYLOCK
        GDALRasterBandH bandh = GDALGetRasterBand(m_datasets[dataset], band_number);
        *scale = GDALGetRasterScale(bandh, success);
        UNLOCK
        SUCCESS
    }

    /**
     * Get the color interpretation of the given band.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band in question
     * @param color_interp The return-slot for the integer-coded color
     *                     interpretation
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_color_interpretation(int dataset, int band_number, int *color_interp)
    {
        TRYLOCK
        GDALRasterBandH bandh = GDALGetRasterBand(m_datasets[dataset], band_number);
        *color_interp = GDALGetRasterColorInterpretation(bandh);
        UNLOCK
        SUCCESS
    }

    /**
     * Get the widths and heights of all overviews associated with the
     * first band of the underlying warped dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band in question
     * @param widths The array in which to return the widths
     * @param heights The array in which to return the heights
     * @param max_length The maximum of number of widths and heights to return
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_overview_widths_heights(int dataset, int band_number, int *widths, int *heights, int max_length)
    {
        TRYLOCK
        GDALRasterBandH band = GDALGetRasterBand(m_datasets[dataset], band_number);
        int overview_count = GDALGetOverviewCount(band);
        for (int i = 0; i < overview_count && i < max_length; ++i)
        {
            GDALRasterBandH overview = GDALGetOverview(band, i);
            widths[i] = GDALGetRasterBandXSize(overview);
            heights[i] = GDALGetRasterBandYSize(overview);
        }
        for (int i = overview_count; i < max_length; ++i)
        {
            widths[i] = heights[i] = -1;
        }
        UNLOCK
        SUCCESS
    }

    /**
     * Get the CRS of the underlying warped dataset in PROJ.4.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param crs The location at-which to return the PROJ.4 string
     * @param max_size The maximum PROJ.4 string size
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_crs_proj4(int dataset, char *crs, int max_size)
    {
        TRYLOCK
        char *result;
        OGRSpatialReferenceH ref = OSRNewSpatialReference(GDALGetProjectionRef(m_datasets[dataset]));
        OSRExportToProj4(ref, &result);
        strncpy(crs, result, max_size);
        CPLFree(result);
        OSRDestroySpatialReference(ref);
        UNLOCK
        SUCCESS
    }

    /**
     * Get the CRS of the underlying warped dataset in WKT.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param crs The location at-which to return the WKT string
     * @param max_size The maximum WKT string size
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_crs_wkt(int dataset, char *crs, int max_size)
    {
        TRYLOCK
        strncpy(crs, GDALGetProjectionRef(m_datasets[dataset]), max_size);
        UNLOCK
        SUCCESS
    }

    /**
     * Get the NODATA value associated with a particular band of the
     * underlying warped dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band in question
     * @param nodata The return-location for the nodata value
     * @param success The return slot for the "is there nodata" value
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_band_nodata(int dataset, int band_number, double *nodata, int *success)
    {
        TRYLOCK
        GDALRasterBandH bandh = GDALGetRasterBand(m_datasets[dataset], band_number);
        *nodata = GDALGetRasterNoDataValue(bandh, success);
        UNLOCK
        SUCCESS
    }

    /**
     * Get the data type of a particular band of the underlying warped
     * dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band in question
     * @param data_type The type of the band in question
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_band_data_type(int dataset, int band_number, GDALDataType *data_type)
    {
        TRYLOCK
        GDALRasterBandH bandh = GDALGetRasterBand(m_datasets[dataset], band_number);
        *data_type = GDALGetRasterDataType(bandh);
        UNLOCK
        SUCCESS
    }

    /**
     * Get the number of raster bands in the underlying warped
     * dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_count The return-location for the integer band count
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_band_count(int dataset, int *band_count) const
    {
        TRYLOCK
        *band_count = GDALGetRasterCount(m_datasets[dataset]);
        UNLOCK
        SUCCESS
    }

    /**
     * Get the transform of the underlying warped dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param transform The return-location of the transform
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_transform(int dataset, double transform[6]) const
    {
        TRYLOCK
        GDALGetGeoTransform(m_datasets[dataset], transform);
        UNLOCK
        SUCCESS
    }

    /**
     * Get the width and height of the underlying warped dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param width The return-location for the width
     * @param height The return-location for the height
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_width_height(int dataset, int *width, int *height)
    {
        TRYLOCK
        auto ds = m_datasets[dataset];
        *width = GDALGetRasterXSize(ds);
        *height = GDALGetRasterYSize(ds);
        UNLOCK
        SUCCESS
    }

    /**
     * Get the maximum and minimum values appearing in the requested band.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band to query
     * @param approx_okay A flag indicating whether approximate min
     *                    and max values are okay
     * @param minmax The return-array for the minimum and maximum
     * @param success The return slot for the "success" flag
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_band_max_min(int dataset, int band_number, int approx_okay, double *minmax, int *success)
    {
        TRYLOCK
        GDALRasterBandH band = GDALGetRasterBand(m_datasets[dataset], band_number);
        if (approx_okay)
        {
            GDALComputeRasterMinMax(band, true, minmax);
            *success = true;
        }
        else
        {
            minmax[0] = GDALGetRasterMinimum(band, success);
            if (*success)
            {
                minmax[1] = GDALGetRasterMaximum(band, success);
            }
        }
        UNLOCK
        SUCCESS
    }

    /**
     * Get the list of metadata domain lists.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band to query (zero for the file itself)
     * @param domain_list The return-location for the list of strings
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_metadata_domain_list(int dataset, int band_number, char ***domain_list)
    {
        TRYLOCK
        auto time_before = get_last_errno_timestamp();
        if (band_number == 0)
        {
            // Must be freed with CSLDestroy
            *domain_list = GDALGetMetadataDomainList(m_datasets[dataset]);
        }
        else
        {
            GDALRasterBandH band = GDALGetRasterBand(m_datasets[dataset], band_number);
            // Must be freed with CSLDestroy
            *domain_list = GDALGetMetadataDomainList(band);
        }
        auto time_after = get_last_errno_timestamp();
        UNLOCK
        if (*domain_list != nullptr || (time_before == time_after))
        {
            SUCCESS
        }
        else
        {
            FAILURE
        }
    }

    /**
     * Get the metadata found in a particular metadata domain.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band to query (zero for the file itself)
     * @param domain The metadata domain to query
     * @param list The return-location for the list of strings
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_metadata(int dataset, int band_number, const char *domain, char ***list)
    {
        TRYLOCK
        auto time_before = get_last_errno_timestamp();
        if (band_number == 0)
        {
            *list = GDALGetMetadata(m_datasets[dataset], domain);
        }
        else
        {
            GDALRasterBandH band = GDALGetRasterBand(m_datasets[dataset], band_number);
            *list = GDALGetMetadata(band, domain);
        }
        auto time_after = get_last_errno_timestamp();
        UNLOCK
        if (*list != nullptr || (time_before == time_after))
        {
            SUCCESS
        }
        else
        {
            FAILURE
        }
    }

    /**
     * Get a particular metadata value associated with a key.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_number The band to query (zero for the file itself)
     * @param key The key of the key ⨯ value metadata pair
     * @param doamin The metadata domain to query
     * @param value The return-location for the value of the key ⨯ value pair
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_metadata_item(int dataset, int band_number, const char *key, const char *domain, const char **value)
    {
        TRYLOCK
        if (band_number == 0)
        {
            *value = GDALGetMetadataItem(m_datasets[dataset], key, domain);
        }
        else
        {
            GDALRasterBandH band = GDALGetRasterBand(m_datasets[dataset], band_number);
            *value = GDALGetMetadataItem(band, key, domain);
        }
        UNLOCK
        if (*value != nullptr)
        {
            SUCCESS
        }
        else
        {
            FAILURE
        }
    }

    /**
     * Read pixels from the underlying dataset.  This is more-or-less
     * a direct wrapper of the GDALRasterIO function see
     * https://gdal.org/api/raster_c_api.html?highlight=rasterio#_CPPv419GDALDatasetRasterIO12GDALDatasetH10GDALRWFlagiiiiPvii12GDALDataTypeiPiiii
     * for more information.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param src_window The pixel-space coordinates of the upper-left
     *                   corner of the source window (the first two
     *                   entries) and the width and height of the
     *                   source window (the last two entries)
     * @param dst_window The width and height of the destination buffer
     * @param band_number The band from which to read
     * @param type The datatype of the destination buffer
     * @param data A pointer to the destination buffer
     * @return ATTEMPT_SUCCESSFUL, DATASET_LOCKED, or a negative CPLErrorNum
     */
    int get_pixels(int dataset,
                   const int src_window[4],
                   int dst_window[2],
                   int band_number,
                   GDALDataType type,
                   void *data) const
    {
        GDALRasterBandH band = GDALGetRasterBand(m_datasets[dataset], band_number);

        TRYLOCK
        auto retval = GDALRasterIO(
            band,                         // source band
            GF_Read,                      // mode
            src_window[0], src_window[1], // read offsets
            src_window[2], src_window[3], // read width, height
            data,                         // write buffer
            dst_window[0], dst_window[1], // write width, height
            type,                         // destination type
            0, 0                          // stride
        );
        UNLOCK

        if (retval == CE_None)
        {
            SUCCESS
        }
        else
        {
            FAILURE
        }
    }

    const uri_options_t &uri_options() const
    {
        return m_uri_options;
    }

    /**
     * Is the dataset valid?
     */
    bool valid() const
    {
        auto src = m_datasets[SOURCE];
        auto warped = m_datasets[WARPED];
        return ((src != nullptr) && (warped != nullptr));
    }

    /**
     * Increment the reference count of this dataset.
     */
    void inc()
    {
        m_use_count++;
    }

    /**
     * Decrement the reference count of this dataset.
     */
    void dec()
    {
        m_use_count--;
    }

    /**
     * Answer "true" iff this dataset is unused and safe to delete.
     *
     * @return A boolean meeting the above description
     */
    bool lock_for_deletion()
    {
        // If the lock could not be obtained, return false
        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }
        // If the lock could be obtained but the reference count is
        // not zero, return false
        else if (m_use_count != 0)
        {
            UNLOCK
            return false;
        }
        // Otherwise return true
        return true;
    }

    /**
     * Unlock this dataset (use only if previously locked for deletion).
     */
    void unlock_for_nondeletion()
    {
        UNLOCK
    }

private:
    /**
     * A function to open a GDAL dataset answering the given warp
     * options.  Should only be called from constructors.
     */
    void open()
    {
        if (pthread_mutex_lock(&m_dataset_lock) != 0)
        {
            fprintf(stderr, "%s%d\n", __FILE__, __LINE__);
            m_datasets[SOURCE] = m_datasets[WARPED] = nullptr;
            return;
        }
        auto src = m_datasets[SOURCE];
        auto warped = m_datasets[WARPED];
        if (src == nullptr || warped == nullptr)
        {
            auto uri = m_uri_options.first;
            auto options_vector = m_uri_options.second;
            char const *options_array[1 << 8];
            GDALWarpAppOptions *app_options = nullptr;

            unsigned int i = 0;
            for (i = 0; i < options_vector.size(); ++i)
            {
                options_array[i] = options_vector[i].c_str();
            }
            options_array[i++] = "-of";
            options_array[i++] = "VRT";
            options_array[i++] = nullptr;
            app_options = GDALWarpAppOptionsNew(const_cast<char **>(options_array), nullptr);
            if (app_options == nullptr)
            {
                // Lock intentionally not unlocked.  The underlying
                // dataset is not valid, so prevent this wrapper from
                // being used.
                m_datasets[SOURCE] = m_datasets[WARPED] = nullptr;
                return;
            }

            m_datasets[SOURCE] = GDALOpen(uri.c_str(), GA_ReadOnly);
            if (m_datasets[SOURCE] == nullptr)
            {
                GDALWarpAppOptionsFree(app_options);
                m_datasets[SOURCE] = m_datasets[WARPED] = nullptr;
                return; // Lock intentionally not unlocked
            }

            m_datasets[WARPED] = GDALWarp("", nullptr, 1, &m_datasets[SOURCE], app_options, 0);
            if (m_datasets[SOURCE] == nullptr)
            {
                GDALClose(m_datasets[SOURCE]);
                GDALWarpAppOptionsFree(app_options);
                m_datasets[SOURCE] = m_datasets[WARPED] = nullptr;
                return; // Lock intentionally not unlocked
            }

            GDALWarpAppOptionsFree(app_options);
        }
        UNLOCK
    }

    /**
     * A function to close the datasets wrapped by this object.
     * Should only be called in moves or the destrutor.
     */
    void close()
    {
        if (valid())
        {
            if (m_datasets[WARPED] != nullptr)
            {
                GDALClose(m_datasets[WARPED]);
                m_datasets[WARPED] = nullptr;
            }
            if (m_datasets[SOURCE] != nullptr)
            {
                GDALClose(m_datasets[SOURCE]);
                m_datasets[SOURCE] = nullptr;
            }
        }
    }

public:
    static const int SOURCE = 0;
    static const int WARPED = 1;

private:
    GDALDatasetH m_datasets[2];
    uri_options_t m_uri_options;
    mutable pthread_mutex_t m_dataset_lock;
    atomic_int_t m_use_count;
};

namespace std
{
template <>
struct hash<locked_dataset>
{
    size_t operator()(const locked_dataset &rhs) const
    {
        auto h = uri_options_hash_t();
        return h(rhs.uri_options());
    }
};
} // namespace std

#undef TRYLOCK
#undef UNLOCK
#undef SUCCESS
#undef FAILURE

#endif
