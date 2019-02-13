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
#include <pthread.h>

#include <vector>
#include <functional>
#include <string>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "gdal.h"
#include "gdal_utils.h"

// Strings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
char *options[] = {
    "-tap", "-tr", "7", "11",
    "-r", "bilinear",
    "-t_srs", "epsg:3857"};
const char *temp_template = "/tmp/rawbench.%d.%d.tif";
#pragma GCC diagnostic pop

// Constants
constexpr int WINDOW_SIZE = (1 << 8);
constexpr int TILE_SIZE = (1 << 8);
constexpr int N = (1 << 10);
constexpr int PATH_LEN = (1 << 6);

// Threads
int lg_steps = 10;
pthread_t threads[N];

// Data
GDALWarpAppOptions *app_options = nullptr;
GDALDatasetH source = nullptr;
GDALDatasetH dataset = nullptr;
GDALRasterBandH band = nullptr;
int width = -1;
int height = -1;
int x = -1;
int y = -1;

// Temporary file name
char temp_path[PATH_LEN];

// Randomness
namespace r = boost::random;

// Expected
auto expected = std::vector<std::size_t>();
auto hash = std::hash<std::string>();

// ANSI
// Reference: https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c
// Reference: https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
#define ANSI_COLOR_RED     "\x1b[31;1m"
#define ANSI_COLOR_GREEN   "\x1b[32;1m"
#define ANSI_COLOR_YELLOW  "\x1b[33;1m"
#define ANSI_COLOR_BLUE    "\x1b[34;1m"
#define ANSI_COLOR_MAGENTA "\x1b[35;1m"
#define ANSI_COLOR_CYAN    "\x1b[36;1m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void *reader(void *)
{
    r::mt19937 generator;
    r::uniform_int_distribution<> x_dist(0, x - 1);
    r::uniform_int_distribution<> y_dist(0, y - 1);

    for (int k = 0; k < (1 << lg_steps); ++k)
    {
        uint8_t buffer[TILE_SIZE * TILE_SIZE + 1];
        buffer[TILE_SIZE * TILE_SIZE] = 0;
        auto i = x_dist(generator);
        auto j = y_dist(generator);
        auto retval = GDALRasterIO(
            band,                             // source band
            GF_Read,                          // mode
            i * WINDOW_SIZE, j * WINDOW_SIZE, // read offsets
            WINDOW_SIZE, WINDOW_SIZE,         // read width, height
            buffer,                           // write buffer
            TILE_SIZE, TILE_SIZE,             // write width, height
            GDT_Byte,                         // destination type
            0, 0                              // stride
        );

        if (retval != CE_None)
        {
            exit(-1);
        }

        auto s = std::string(reinterpret_cast<char *>(buffer));
        auto h = hash(s);
        assert(h == expected[i + j * x]);
    }

    return nullptr;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        exit(-1);
    }
    if (argc >= 3)
    {
        sscanf(argv[2], "%d", &lg_steps);
    }

    // Initialize
    GDALAllRegister();

    // Setup
    app_options = GDALWarpAppOptionsNew(options, nullptr);
    source = GDALOpen(argv[1], GA_ReadOnly);
    sprintf(temp_path, temp_template, getpid(), 0);
    dataset = GDALWarp(temp_path, nullptr, 1, &source, app_options, 0);
    band = GDALGetRasterBand(dataset, 1);
    width = GDALGetRasterXSize(dataset);
    height = GDALGetRasterYSize(dataset);
    x = (width / WINDOW_SIZE) - 1;
    y = (height / WINDOW_SIZE) - 1;

    // Expected
    fprintf(stdout, ANSI_COLOR_GREEN "Computing expected results\n" ANSI_COLOR_RESET);
    expected.resize(x * y);
    for (int i = 0; i < x; ++i)
    {
        for (int j = 0; j < y; ++j)
        {
            uint8_t buffer[TILE_SIZE * TILE_SIZE + 1];
            buffer[TILE_SIZE * TILE_SIZE] = 0;
            auto retval = GDALRasterIO(
                band,                             // source band
                GF_Read,                          // mode
                i * WINDOW_SIZE, j * WINDOW_SIZE, // read offsets
                WINDOW_SIZE, WINDOW_SIZE,         // read width, height
                buffer,                           // write buffer
                TILE_SIZE, TILE_SIZE,             // write width, height
                GDT_Byte,                         // destination type
                0, 0                              // stride
            );

            if (retval != CE_None)
            {
                exit(-1);
            }

            auto s = std::string(reinterpret_cast<char *>(buffer));
            expected[i + j * x] = hash(s);
        }
    }

    GDALClose(dataset);
    unlink(temp_path);
    sprintf(temp_path, temp_template, getpid(), 1);
    dataset = GDALWarp(temp_path, nullptr, 1, &source, app_options, 0);
    band = GDALGetRasterBand(dataset, 1);

    // Reuse one dataset
    fprintf(stdout, ANSI_COLOR_GREEN "Checking results\n" ANSI_COLOR_RESET);
    for (int i = 0; i < N; ++i)
    {
        assert(pthread_create(&threads[i], nullptr, reader, 0) == 0);
    }
    for (int i = 0; i < N; ++i)
    {
        assert(pthread_join(threads[i], nullptr) == 0);
        fprintf(stdout, ANSI_COLOR_MAGENTA "." ANSI_COLOR_RESET);
    }
    fprintf(stdout, "\n");

    // Cleanup
    GDALWarpAppOptionsFree(app_options);
    GDALClose(dataset);
    GDALClose(source);
    unlink(temp_path);

    return 0;
}
