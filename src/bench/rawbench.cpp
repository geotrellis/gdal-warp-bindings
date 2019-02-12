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
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "gdal.h"
#include "gdal_utils.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
char *options[] = {
    "-tap", "-tr", "33", "33",
    "-r", "bilinear",
    "-t_srs", "epsg:3857"};
const char *temp_template = "/tmp/rawbench.%d.%d.vrt";
#pragma GCC diagnostic pop

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        exit(-1);
    }

    GDALAllRegister();

    char vrt_path[1 << 6];
    sprintf(vrt_path, temp_template, getpid(), 0);

    GDALWarpAppOptions *app_options = GDALWarpAppOptionsNew(options, nullptr);
    GDALDatasetH source = GDALOpen(argv[1], GA_ReadOnly);
    GDALDatasetH dataset = GDALWarp(vrt_path, nullptr, 1, &source, app_options, 0);

    fprintf(stderr, "%d %d %d\n",
            GDALGetRasterCount(dataset), GDALGetRasterXSize(dataset), GDALGetRasterYSize(dataset));

    GDALWarpAppOptionsFree(app_options);
    unlink(vrt_path);

    return 0;
}
