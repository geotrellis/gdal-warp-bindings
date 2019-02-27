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

#include <pthread.h>

#include <gdal.h>
#include <gdal_utils.h>

#include "types.hpp"

class locked_dataset
{
  public:
    locked_dataset()
        : p_dataset(nullptr),
          p_source(nullptr),
          m_uri_options(),
          m_lock(PTHREAD_MUTEX_INITIALIZER),
          m_use_count(PTHREAD_RWLOCK_INITIALIZER)
    {
    }

    locked_dataset(const uri_options_t &uri_options)
        : p_dataset(nullptr),
          p_source(nullptr),
          m_uri_options(uri_options),
          m_lock(PTHREAD_MUTEX_INITIALIZER),
          m_use_count(PTHREAD_RWLOCK_INITIALIZER)
    {
        open();
    }

    locked_dataset(const locked_dataset &rhs)
        : p_dataset(nullptr), p_source(nullptr),
          m_uri_options(rhs.m_uri_options),
          m_lock(PTHREAD_MUTEX_INITIALIZER),
          m_use_count(PTHREAD_RWLOCK_INITIALIZER)
    {
#ifdef DEBUG
        fprintf(stderr, "COPY CONSTRUCTOR\n");
#endif
        open();
    }

    locked_dataset(locked_dataset &&rhs) noexcept
        : p_dataset(std::exchange(rhs.p_dataset, nullptr)),
          p_source(std::exchange(rhs.p_source, nullptr)),
          m_uri_options(std::exchange(rhs.m_uri_options, uri_options_t())),
          m_lock(std::exchange(rhs.m_lock, PTHREAD_MUTEX_INITIALIZER)),
          m_use_count(std::exchange(rhs.m_use_count, PTHREAD_RWLOCK_INITIALIZER))
    {
        // XXX not thread safe, but the rhs should always be a
        // just-created local that is not in use.
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

        p_dataset = rhs.p_dataset;
        p_source = rhs.p_source;
        m_lock = rhs.m_lock;
        m_use_count = rhs.m_use_count;
        m_uri_options = std::move(rhs.m_uri_options);

        rhs.p_dataset = nullptr;
        rhs.p_source = nullptr;
        rhs.m_lock = PTHREAD_MUTEX_INITIALIZER;
        rhs.m_use_count = PTHREAD_RWLOCK_INITIALIZER;
        rhs.m_uri_options = uri_options_t();

        return *this;
        // XXX not thread safe, but the rhs should always be a
        // just-created local that is not in use.
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
     * Get the number of raster bands in the underlying warped dataset.
     *
     * @param band_count The return-location for the integer band count
     * @return A boolean: True iff the operation succeeded
     */
    bool get_band_count(int * band_count) const
    {
        if (pthread_mutex_trylock(&m_lock) != 0)
        {
            return false;
        }
        *band_count = GDALGetRasterCount(p_dataset);
        pthread_mutex_unlock(&m_lock);
        return true;
    }

    /**
     * Get the transform of the underlying warped dataset.
     *
     * @param transform The return-location of the transform
     * @return A boolean: True iff the operation succeeded
     */
    bool get_transform(double transform[6]) const
    {
        if (pthread_mutex_trylock(&m_lock) != 0)
        {
            return false;
        }
        GDALGetGeoTransform(p_dataset, transform);
        pthread_mutex_unlock(&m_lock);
        return true;
    }

    /**
     * Get the width and height of the underlying warped dataset.
     *
     * @param width The return-location for the width
     * @param height The return-location for the height
     * @return A boolean: True iff the operation succeed
     */
    bool get_width_height(int *width, int *height)
    {
        if (pthread_mutex_trylock(&m_lock) != 0)
        {
            return false;
        }

        *width = GDALGetRasterXSize(p_dataset);
        *height = GDALGetRasterYSize(p_dataset);
        pthread_mutex_unlock(&m_lock);

        return true;
    }

    /**
     * Read pixels from the underlying dataset.  This is more-or-less
     * a direct wrapper of the GDALRasterIO function, so see
     * https://www.gdal.org/gdal_8h.html#afb94984e55f110ec5346fc7ab6a139ef
     * for more information.
     *
     * @param src_window The pixel-space coordinates of the upper-left
     *                   corner of the source window (the first two
     *                   entries) and the width and height of the
     *                   source window (the last two entries)
     * @param dst_window The width and height of the destination buffer
     * @param band_number The band from which to read
     * @param type The datatype of the destination buffer
     * @param data A pointer to the destination buffer
     * @return A boolean: True iff the read succeeded
     */
    bool get_pixels(const int src_window[4],
                    int dst_window[2],
                    int band_number,
                    GDALDataType type,
                    void *data) const
    {
        GDALRasterBandH band = GDALGetRasterBand(p_dataset, band_number);

        if (pthread_mutex_trylock(&m_lock) != 0)
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
        pthread_mutex_unlock(&m_lock);

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
        return ((p_source != nullptr) && (p_dataset != nullptr));
    }

    /**
     * Increment the reference count of this dataset.
     *
     * @return A boolean which is true iff the increment succeeded
     */
    bool inc()
    {
        if (pthread_rwlock_tryrdlock(&m_use_count) != 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    /**
     * Decrement the reference count of this dataset.
     */
    void dec()
    {
        pthread_rwlock_unlock(&m_use_count);
    }

    /**
     * Answer "true" iff this dataset is unused and safe to delete.
     *
     * @return A boolean meeting the above description
     */
    bool lock_for_deletion()
    {
        if (pthread_rwlock_trywrlock(&m_use_count) != 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    /**
     * Unlock this dataset (use only if previously locked for deletion).
     */
    void unlock()
    {
        pthread_rwlock_unlock(&m_use_count);
    }

  private:
    /**
     * A function to open a GDAL dataset answering the given warp
     * options.  Should only be called from constructors.
     */
    void open()
    {
        pthread_mutex_lock(&m_lock);
        if (p_source == nullptr || p_dataset == nullptr)
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
                p_source = p_dataset = nullptr;
                return;
            }

            p_source = GDALOpen(uri.c_str(), GA_ReadOnly);
            if (p_source == nullptr)
            {
                p_source = p_dataset = nullptr;
                return; // Lock intentionally not unlocked
            }

            p_dataset = GDALWarp("/dev/null", nullptr, 1, &p_source, app_options, 0);
            if (p_dataset == nullptr)
            {
                p_source = p_dataset = nullptr;
                return; // Lock intentionally not unlocked
            }

            GDALWarpAppOptionsFree(app_options);
        }
        pthread_mutex_unlock(&m_lock);
    }

    /**
     * A function to close the datasets wrapped by this object.
     * Should only be called in moves or the destrucdtor.
     */
    void close()
    {
        pthread_mutex_lock(&m_lock);
        if (p_dataset != nullptr)
        {
            GDALClose(p_dataset);
            p_dataset = nullptr;
        }
        if (p_source != nullptr)
        {
            GDALClose(p_source);
            p_source = nullptr;
        }
        pthread_mutex_unlock(&m_lock);
    }

  private:
    GDALDatasetH p_dataset;
    GDALDatasetH p_source;
    uri_options_t m_uri_options;
    mutable pthread_mutex_t m_lock;
    mutable pthread_rwlock_t m_use_count;
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
