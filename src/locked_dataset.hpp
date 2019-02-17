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
        : p_dataset(nullptr), p_source(nullptr),
          m_lock(PTHREAD_MUTEX_INITIALIZER), m_uri_options()
    {
    }

    locked_dataset(const uri_options_t &uri_options)
        : p_dataset(nullptr), p_source(nullptr),
          m_lock(PTHREAD_MUTEX_INITIALIZER), m_uri_options(uri_options)
    {
        open();
    }

    locked_dataset(const locked_dataset &rhs)
        : p_dataset(nullptr), p_source(nullptr),
          m_lock(PTHREAD_MUTEX_INITIALIZER), m_uri_options(rhs.m_uri_options)
    {
#ifdef DEBUG
        fprintf(stderr, "COPY CONSTRUCTOR\n");
#endif
        open();
    }

    locked_dataset(locked_dataset &&rhs) noexcept
        : p_dataset(std::exchange(rhs.p_dataset, nullptr)),
          p_source(std::exchange(rhs.p_source, nullptr)),
          m_lock(std::exchange(rhs.m_lock, PTHREAD_MUTEX_INITIALIZER)),
          m_uri_options(std::exchange(rhs.m_uri_options, uri_options_t()))
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
        m_uri_options = std::move(rhs.m_uri_options);

        rhs.p_dataset = nullptr;
        rhs.p_source = nullptr;
        rhs.m_lock = PTHREAD_MUTEX_INITIALIZER;
        rhs.m_uri_options = uri_options_t();

        return *this;
        // XXX not thread safe, but the rhs should always be a
        // just-created local that is not in use.
    }

    ~locked_dataset()
    {
        close();
    }

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
            throw std::exception(); // XXX just make invalid?
        }

        return true;
    }

    bool valid() const
    {
        return ((p_source != nullptr) && (p_dataset != nullptr));
    }

  private:
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

            p_source = GDALOpen(uri.c_str(), GA_ReadOnly);
            p_dataset = GDALWarp("/dev/null", nullptr, 1, &p_source, app_options, 0);
            GDALWarpAppOptionsFree(app_options);
        }
        pthread_mutex_unlock(&m_lock);
    }

  private:
    GDALDatasetH p_dataset;
    GDALDatasetH p_source;
    mutable pthread_mutex_t m_lock;
    uri_options_t m_uri_options;
};

#endif
