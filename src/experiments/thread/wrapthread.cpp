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
#include <boost/timer/timer.hpp>

#include <gdal.h>
#include <gdal_utils.h>

#include "../../bindings.h"

// Strings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
const char *xres = "5";
const char *yres = "7";
char const *expected_options[] = {
    "-of", "VRT",
    "-tap", "-tr", xres, yres,
    "-r", "bilinear",
    "-t_srs", "epsg:3857",
    nullptr};
char const *actual_options[] = {
    "-tap", "-tr", xres, yres,
    "-r", "bilinear",
    "-t_srs", "epsg:3857",
    nullptr};
#pragma GCC diagnostic pop

// Constants
constexpr int WINDOW_SIZE = (1 << 8);
constexpr int TILE_SIZE = (1 << 8);
constexpr int N = (1 << 10);
constexpr int PATH_LEN = (1 << 6);
constexpr int COPIES = -4;

// Threads
int lg_steps = 12;
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
uint64_t token = -1;

// Randomness
namespace r = boost::random;

// Timer
namespace t = boost::timer;

// Expected
auto expected = std::vector<std::size_t>();
auto hash = std::hash<std::string>();

// ANSI
// Reference: https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c
// Reference: https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
#define ANSI_COLOR_RED "\x1b[31;1m"
#define ANSI_COLOR_GREEN "\x1b[32;1m"
#define ANSI_COLOR_YELLOW "\x1b[33;1m"
#define ANSI_COLOR_BLUE "\x1b[34;1m"
#define ANSI_COLOR_MAGENTA "\x1b[35;1m"
#define ANSI_COLOR_CYAN "\x1b[36;1m"
#define ANSI_COLOR_RESET "\x1b[0m"

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
        int src_window[4] = {i * WINDOW_SIZE, j * WINDOW_SIZE, WINDOW_SIZE, WINDOW_SIZE};
        int dst_window[2] = {TILE_SIZE, TILE_SIZE};

        if (get_data(token, 1, 0, 0, COPIES, src_window, dst_window, 1, GDT_Byte, buffer) > 0)
        {
            auto s = std::string(reinterpret_cast<char *>(buffer));
            auto h = hash(s);
            assert(h == expected[i + j * x]);
        }
        else
        {
            assert(false);
        }
    }

    return nullptr;
}

bool keep_going = true;

void *leaker(void *argv1)
{
    auto source = GDALOpen(static_cast<const char *>(argv1), GA_ReadOnly);
    auto dataset = GDALWarp("/dev/null", nullptr, 1, &source, app_options, 0);
    while (keep_going)
    {
        sleep(1);
    }
    return dataset;
}

int main(int argc, char **argv)
{
    int n = N;

    if (argc < 2)
    {
        exit(-1);
    }
    if (argc >= 3)
    {
        sscanf(argv[2], "%d", &lg_steps);
        fprintf(stderr, ANSI_COLOR_BLUE "lg_steps = %d\n" ANSI_COLOR_RESET, lg_steps);
    }
    if (argc >= 4)
    {
        sscanf(argv[3], "%d", &n);
        if (n > N)
        {
            n = N;
        }
        else if (n < -N)
        {
            n = -N;
        }
        fprintf(stderr, ANSI_COLOR_BLUE "n = %d\n" ANSI_COLOR_RESET, n);
    }

    // Initialize
    GDALAllRegister();

    // Setup
    app_options = GDALWarpAppOptionsNew(const_cast<char **>(expected_options), nullptr);
    source = GDALOpen(argv[1], GA_ReadOnly);
    {
        t::auto_cpu_timer timer;

        fprintf(stdout, ANSI_COLOR_GREEN "DATASET\n" ANSI_COLOR_RESET);
        dataset = GDALWarp("/dev/null", nullptr, 1, &source, app_options, 0);
        band = GDALGetRasterBand(dataset, 1);
    }
    width = GDALGetRasterXSize(dataset);
    height = GDALGetRasterYSize(dataset);
    x = (width / WINDOW_SIZE) - 1;
    y = (height / WINDOW_SIZE) - 1;

    // Expected
    expected.resize(x * y);
    {
        t::auto_cpu_timer timer;

        fprintf(stdout, ANSI_COLOR_GREEN "EXPECTED RESULTS\n" ANSI_COLOR_RESET);
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
    }

    // Cleanup
    GDALClose(dataset);
    GDALClose(source);

    init(1 << 8);
    token = get_token(argv[1], const_cast<const char **>(actual_options));
    {
        t::auto_cpu_timer timer;

        fprintf(stdout, ANSI_COLOR_GREEN "ACTUAL RESULTS\n" ANSI_COLOR_RESET);
        if (n > 0)
        {
            for (int i = 0; i < n; ++i)
            {
                assert(pthread_create(&threads[i], nullptr, reader, 0) == 0);
            }
            for (int i = 0; i < n; ++i)
            {
                assert(pthread_join(threads[i], nullptr) == 0);
                fprintf(stdout, ANSI_COLOR_MAGENTA "." ANSI_COLOR_RESET);
            }
            fprintf(stdout, "\n");
        }
        else
        {
            // Leak some datasets
            for (int i = 0; i < -n; ++i)
            {
                fprintf(stderr, ANSI_COLOR_RED "?" ANSI_COLOR_RESET);
                assert(pthread_create(&threads[i], nullptr, leaker, argv[1]) == 0);
            }
            fprintf(stderr, "\n");
            reader(nullptr);
            keep_going = false;
            for (int i = 0; i < -n; ++i)
            {
                assert(pthread_join(threads[i], nullptr) == 0);
                fprintf(stderr, ANSI_COLOR_BLUE "?" ANSI_COLOR_RESET);
            }
            fprintf(stderr, "\n");
        }
    }

    deinit();
    GDALWarpAppOptionsFree(app_options);
    GDALDestroyDriverManager();

    return 0;
}
