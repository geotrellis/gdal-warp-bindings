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

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <random>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "../../bindings.h"
#include "../../locked_dataset.hpp"

// Strings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
const char *xres = "5";
const char *yres = "7";
char const *bad_uri = "HOPEFULLY_THERE_IS_NO_FILE_WITH_THIS_NAME";
char const *good_uris[] = {
    "/tmp/A.tif", "/tmp/B.tif", "/tmp/C.tif",
    "/tmp/D.tif", "/tmp/E.tif", "/tmp/F.tif",
    "/tmp/G.tif", "/tmp/H.tif", "/tmp/I.tif"};
char const *options[] = {
    "-tap", "-tr", xres, yres,
    "-r", "bilinear",
    "-t_srs", "epsg:3857",
    nullptr};
#pragma GCC diagnostic pop

// Constants
constexpr int N = (1024);
constexpr int DIM = 1 << 8;
constexpr int BUFFERSIZE = DIM * DIM;
constexpr int ATTEMPTS = 1 << 20;
constexpr int COPIES = -4;

// Threads
int lg_steps = 12;
pthread_t threads[N];

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

void *reader(void *argv1)
{
    auto g = std::mt19937(std::random_device{}());
    auto dist = std::uniform_int_distribution<int>(0, 999);
    char *buf = static_cast<char *>(malloc(BUFFERSIZE));
    double transform[6];
    int scratch1, scratch2;
    int src_window[4] = {0, 0, DIM, DIM};
    int dst_window[2] = {DIM, DIM};

    for (int k = 0; k < (1 << lg_steps); ++k)
    {
        uint64_t token;
        const char *uri = good_uris[dist(g) % 9];

        token = get_token(uri, options);

        get_crs_wkt(token, token % 2, ATTEMPTS, COPIES, buf, BUFFERSIZE);
        get_crs_proj4(token, token % 2, ATTEMPTS, COPIES, buf, BUFFERSIZE);
        get_band_nodata(token, token % 2, ATTEMPTS, COPIES, 1, transform, &scratch1);
        get_width_height(token, token % 2, ATTEMPTS, COPIES, &scratch1, &scratch2);
        get_data(token, token % 2, ATTEMPTS, COPIES, src_window, dst_window, 1, 1 /* GDT_Byte */, buf);
    }

    return nullptr;
}

bool keep_going = true;

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
        if (n > N || n < 0)
        {
            n = N;
        }
        fprintf(stderr, ANSI_COLOR_BLUE "n = %d\n" ANSI_COLOR_RESET, n);
    }

    // Setup

    init(1 << 2);
    dup2(open("/dev/null", O_WRONLY), 2);

    for (int i = 0; i < n; ++i)
    {
        assert(pthread_create(&threads[i], nullptr, reader, argv[1]) == 0);
    }
    for (int i = 0; i < n; ++i)
    {
        assert(pthread_join(threads[i], nullptr) == 0);
        fprintf(stdout, ANSI_COLOR_MAGENTA "." ANSI_COLOR_RESET);
    }
    fprintf(stdout, "\n");

    deinit();

    return 0;
}
