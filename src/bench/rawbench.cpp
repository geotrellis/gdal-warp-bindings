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
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/timer/timer.hpp>

#include "gdal.h"
#include "gdal_utils.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
char *options[] = {
    "-tap", "-tr", "33", "42",
    "-r", "bilinear",
    "-t_srs", "epsg:3857"};
const char *temp_template = "/tmp/rawbench.%d.%d.vrt";
#pragma GCC diagnostic pop

constexpr int window_size = (1 << 6);
constexpr int tile_size = (1 << 6);

namespace r = boost::random;
namespace t = boost::timer;

int main(int argc, char **argv)
{
    int log_steps = 10;

    if (argc < 2)
    {
        exit(-1);
    }
    if (argc >= 3)
    {
        sscanf(argv[2], "%d", &log_steps);
    }

    // Initialize
    GDALAllRegister();

    // Temporary VRT file name
    char vrt_path[1 << 6];
    sprintf(vrt_path, temp_template, getpid(), 0);

    // Data
    GDALWarpAppOptions *app_options = GDALWarpAppOptionsNew(options, nullptr);
    GDALDatasetH source = GDALOpen(argv[1], GA_ReadOnly);
    GDALDatasetH dataset = GDALWarp(vrt_path, nullptr, 1, &source, app_options, 0);
    GDALRasterBandH band = GDALGetRasterBand(dataset, 1);
    auto x = GDALGetRasterXSize(dataset);
    auto y = GDALGetRasterYSize(dataset);

    // Randomness
    r::mt19937 generator;
    r::uniform_int_distribution<> x_dist(0, x - 1 - window_size);
    r::uniform_int_distribution<> y_dist(0, y - 1 - window_size);

    // Reuse one dataset
    {
        t::auto_cpu_timer timer;

        for (int i = 0; i < (1 << log_steps); ++i)
        {
            uint8_t buffer[tile_size * tile_size];
            auto retval = GDALRasterIO(
                band,                                 // source band
                GF_Read,                              // mode
                x_dist(generator), y_dist(generator), // read offsets
                window_size, window_size,             // read width, height
                buffer,                               // write buffer
                tile_size, tile_size,                 // write width, height
                GDT_Byte,                             // destination type
                0, 0                                  // stride
            );

            if (retval != CE_None)
            {
                exit(-1);
            }
        }
    }

#if 0
    // Reuse a few datasets
    {
        constexpr int N = (1 << 10);
        GDALDatasetH datasets[N];
        GDALRasterBandH bands[N];

        for (int i = 0; i < N; ++i)
        {
            datasets[i] = GDALWarp(vrt_path, nullptr, 1, &source, app_options, 0);
            bands[i] = GDALGetRasterBand(datasets[i], 1);
        }

        t::auto_cpu_timer timer;

        for (int i = 0; i < (1 << log_steps); ++i)
        {
            uint8_t buffer[tile_size * tile_size];
            auto retval = GDALRasterIO(
                bands[i % N],                         // source band
                GF_Read,                              // mode
                x_dist(generator), y_dist(generator), // read offsets
                window_size, window_size,             // read width, height
                buffer,                               // write buffer
                tile_size, tile_size,                 // write width, height
                GDT_Byte,                             // destination type
                0, 0                                  // stride
            );

            if (retval != CE_None)
            {
                exit(-1);
            }
        }

        for (int i = 0; i < N; ++i)
        {
            GDALClose(datasets[i]);
        }
    }

    // Do not reuse datasets
    {
        t::auto_cpu_timer timer;

        for (int i = 0; i < (1 << log_steps); ++i)
        {
            GDALDatasetH dataset = GDALWarp(vrt_path, nullptr, 1, &source, app_options, 0);
            GDALRasterBandH band = GDALGetRasterBand(dataset, 1);
            uint8_t buffer[tile_size * tile_size];
            auto retval = GDALRasterIO(
                band,                                 // source band
                GF_Read,                              // mode
                x_dist(generator), y_dist(generator), // read offsets
                window_size, window_size,             // read width, height
                buffer,                               // write buffer
                tile_size, tile_size,                 // write width, height
                GDT_Byte,                             // destination type
                0, 0                                  // stride
            );

            if (retval != CE_None)
            {
                exit(-1);
            }

            GDALClose(dataset);
        }
    }
#endif

    // Clean-Up
    GDALWarpAppOptionsFree(app_options);
    GDALClose(dataset);
    GDALClose(source);
    unlink(vrt_path);

    return 0;
}
