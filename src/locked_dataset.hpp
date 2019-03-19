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

#ifndef __locked_dataset_H__
#define __locked_dataset_H__

#include <functional>

#ifdef DEBUG
#include <cstdio>
#endif

#if defined(__MINGW32__)
typedef _locale_t locale_t;
#endif

#include <atomic>
#include <cstring>

#include <pthread.h>

#include <gdal.h>
#include <gdal_utils.h>
#include <cpl_conv.h>
#include <ogr_srs_api.h>

#include "types.hpp"

typedef std::atomic<int> atomic_int_t;

class locked_dataset
{
  public:
    locked_dataset()
        : m_datasets{nullptr, nullptr},
          m_uri_options(),
          m_dataset_lock(PTHREAD_MUTEX_INITIALIZER),
          m_use_count(0)
    {
    }

    locked_dataset(const uri_options_t &uri_options)
        : m_datasets{nullptr, nullptr},
          m_uri_options(uri_options),
          m_dataset_lock(PTHREAD_MUTEX_INITIALIZER),
          m_use_count(0)
    {
        open();
    }

    locked_dataset(const locked_dataset &rhs)
        : m_datasets{nullptr, nullptr},
          m_uri_options(rhs.m_uri_options),
          m_dataset_lock(PTHREAD_MUTEX_INITIALIZER),
          m_use_count(static_cast<int>(rhs.m_use_count))
    {
#ifdef DEBUG
        fprintf(stderr, "COPY CONSTRUCTOR\n");
#endif
        open();
    }

    locked_dataset(locked_dataset &&rhs) noexcept
        : m_uri_options(std::exchange(rhs.m_uri_options, uri_options_t())),
          m_dataset_lock(std::exchange(rhs.m_dataset_lock, PTHREAD_MUTEX_INITIALIZER))
    {
        // XXX not thread safe, but the rhs should always be a
        // just-created local that is not in use anywhere else.
        m_use_count = static_cast<int>(rhs.m_use_count);
        rhs.m_use_count = 0;
        m_datasets[SOURCE] = std::exchange(rhs.m_datasets[SOURCE], nullptr);
        m_datasets[WARPED] = std::exchange(rhs.m_datasets[WARPED], nullptr);
    }

    locked_dataset &operator=(const locked_dataset &rhs)
    {
        close();
        *this = locked_dataset(rhs.m_uri_options);
#ifdef DEBUG
        fprintf(stderr, "ASSIGNMENT OPERATOR\n");
#endif
        return *this;
    }

    locked_dataset &operator=(locked_dataset &&rhs) noexcept
    {
        close();

        m_datasets[SOURCE] = std::exchange(rhs.m_datasets[SOURCE], nullptr);
        m_datasets[WARPED] = std::exchange(rhs.m_datasets[WARPED], nullptr);
        m_dataset_lock = std::exchange(rhs.m_dataset_lock, PTHREAD_MUTEX_INITIALIZER);
        m_use_count = static_cast<int>(rhs.m_use_count);
        rhs.m_use_count = 0;
        m_uri_options = std::exchange(rhs.m_uri_options, uri_options_t());

        return *this;
        // XXX not thread safe, but the rhs should always be a
        // just-created local that is not in use anywhere else.
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
     * Get the widths and heights of all overviews associated with the
     * first band of the underlying warped dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param widths The array in which to return the widths
     * @param heights The array in which to return the heights
     * @param max_length The maximum of number of widths and heights to return
     * @return True iff the operation succeeded
     */
    bool get_overview_widths_heights(int dataset, int *widths, int *heights, int max_length)
    {
        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }
        GDALRasterBandH band = GDALGetRasterBand(m_datasets[dataset], 1);
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
        pthread_mutex_unlock(&m_dataset_lock);
        return true;
    }

    /**
     * Get the CRS of the underlying warped dataset in PROJ.4.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param crs The location at-which to return the PROJ.4 string
     * @param max_size The maximum PROJ.4 string size
     * @return True iff the operation succeeded
     */
    bool get_crs_proj4(int dataset, char *crs, int max_size)
    {
        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }
        char *result;
        OGRSpatialReferenceH ref = OSRNewSpatialReference(GDALGetProjectionRef(m_datasets[dataset]));
        OSRExportToProj4(ref, &result);
        strncpy(crs, result, max_size);
        CPLFree(result);
        pthread_mutex_unlock(&m_dataset_lock);
        return true;
    }

    /**
     * Get the CRS of the underlying warped dataset in WKT.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param crs The location at-which to return the WKT string
     * @param max_size The maximum WKT string size
     * @return True iff the operation succeeded
     */
    bool get_crs_wkt(int dataset, char *crs, int max_size)
    {
        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }
        strncpy(crs, GDALGetProjectionRef(m_datasets[dataset]), max_size);
        pthread_mutex_unlock(&m_dataset_lock);
        return true;
    }

    /**
     * Get the NODATA value associated with a particular band of the
     * underlying warped dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band The band in question
     * @param nodata The return-location for the nodata value
     * @param success The return slot for the "is there nodata" value
     * @return True iff the operation succeeded
     */
    bool get_band_nodata(int dataset, int band, double *nodata, int *success)
    {
        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }
        GDALRasterBandH bandh = GDALGetRasterBand(m_datasets[dataset], band);
        *nodata = GDALGetRasterNoDataValue(bandh, success);
        pthread_mutex_unlock(&m_dataset_lock);
        return true;
    }

    /**
     * Get the data type of a particular band of the underlying warped
     * dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band The band in question
     * @param data_type The type of the band in question
     * @return True iff the operation succeeded
     */
    bool get_band_data_type(int dataset, int band, GDALDataType *data_type)
    {
        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }
        GDALRasterBandH bandh = GDALGetRasterBand(m_datasets[dataset], band);
        *data_type = GDALGetRasterDataType(bandh);
        pthread_mutex_unlock(&m_dataset_lock);
        return true;
    }

    /**
     * Get the number of raster bands in the underlying warped
     * dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param band_count The return-location for the integer band count
     * @return True iff the operation succeeded
     */
    bool get_band_count(int dataset, int *band_count) const
    {
        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }
        *band_count = GDALGetRasterCount(m_datasets[dataset]);
        pthread_mutex_unlock(&m_dataset_lock);
        return true;
    }

    /**
     * Get the transform of the underlying warped dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param transform The return-location of the transform
     * @return True iff the operation succeeded
     */
    bool get_transform(int dataset, double transform[6]) const
    {
        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }
        GDALGetGeoTransform(m_datasets[dataset], transform);
        pthread_mutex_unlock(&m_dataset_lock);
        return true;
    }

    /**
     * Get the width and height of the underlying warped dataset.
     *
     * @param dataset The index of the dataset (source == 0, warped == 1)
     * @param width The return-location for the width
     * @param height The return-location for the height
     * @return True iff the operation succeed
     */
    bool get_width_height(int dataset, int *width, int *height)
    {
        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }

        auto ds = m_datasets[dataset];
        *width = GDALGetRasterXSize(ds);
        *height = GDALGetRasterYSize(ds);
        pthread_mutex_unlock(&m_dataset_lock);

        return true;
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
     * @return True iff the operation succeeded
     */
    bool get_band_max_min(int dataset, int band_number, int approx_okay, double *minmax, int *success)
    {
        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }

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
        pthread_mutex_unlock(&m_dataset_lock);

        return true;
    }

    /**
     * Read pixels from the underlying dataset.  This is more-or-less
     * a direct wrapper of the GDALRasterIO function, so see
     * https://www.gdal.org/gdal_8h.html#afb94984e55f110ec5346fc7ab6a139ef
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
     * @return True iff the read succeeded
     */
    bool get_pixels(int dataset,
                    const int src_window[4],
                    int dst_window[2],
                    int band_number,
                    GDALDataType type,
                    void *data) const
    {
        GDALRasterBandH band = GDALGetRasterBand(m_datasets[dataset], band_number);

        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }

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
        pthread_mutex_unlock(&m_dataset_lock);

        if (retval != CE_None)
        {
            return false;
        }

        return true;
    }

    const uri_options_t &uri_options() const
    {
        return m_uri_options;
    }

    bool valid() const
    {
        auto src = m_datasets[SOURCE];
        auto warped = m_datasets[WARPED];
        return ((src != nullptr) && (warped != nullptr));
    }

    /**
     * Increment the reference count of this dataset.
     *
     * @return A boolean which is true iff the increment succeeded
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
        if (pthread_mutex_trylock(&m_dataset_lock) != 0)
        {
            return false;
        }
        else if (m_use_count != 0)
        {
            pthread_mutex_unlock(&m_dataset_lock);
            return false;
        }
        return true;
    }

    /**
     * Unlock this dataset (use only if previously locked for deletion).
     */
    void unlock_for_nondeletion()
    {
        pthread_mutex_unlock(&m_dataset_lock);
    }

    /**
     * Prepare for deletion (use only if previously locked for deletion).
     */
    void prepare_for_overwrite()
    {
        pthread_mutex_unlock(&m_dataset_lock);
    }

  private:
    /**
     * A function to open a GDAL dataset answering the given warp
     * options.  Should only be called from constructors.
     */
    void open()
    {
        pthread_mutex_lock(&m_dataset_lock);
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
        pthread_mutex_unlock(&m_dataset_lock);
    }

    /**
     * A function to close the datasets wrapped by this object.
     * Should only be called in moves or the destrucdtor.
     */
    void close()
    {
        if (valid())
        {
            pthread_mutex_lock(&m_dataset_lock);
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
            pthread_mutex_unlock(&m_dataset_lock);
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

#endif
