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

#ifndef __BINDINGS_H__
#define __BINDINGS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void init(size_t size);
    void deinit();

    uint64_t get_token(const char *uri, const char **options);

    int get_block_size(uint64_t token, int dataset, int attempts, int copies,
                       int band_number, int *width, int *height);

    int get_histogram(uint64_t token, int dataset, int attempts, int copies,
                      int band_number,
                      double lower, double upper, int num_buckets,
                      unsigned long long int *hist,
                      int include_out_of_range, int approx_ok);

    int get_offset(uint64_t token, int dataset, int attempts, int copies,
                   int band_number, double *offset, int *success);

    int get_scale(uint64_t token, int dataset, int attempts, int copies,
                  int band_number, double *scale, int *success);

    int noop(uint64_t token, int dataset, int attempts, int copies);

    int get_color_interpretation(uint64_t token, int dataset, int attempts, int copies,
                                 int band_number, int *color_interp);

    int get_metadata_domain_list(uint64_t token, int dataset, int attempts, int copies,
                                 int band_number, char ***domain_list);

    int get_metadata(uint64_t token, int dataset, int attempts, int copies,
                     int band_number, const char *domain, char ***list);

    int get_metadata_item(uint64_t token, int dataset, int attempts, int copies,
                          int band_number, const char *key, const char *domain, const char **value);

    int get_overview_widths_heights(uint64_t token, int dataset, int attempts, int copies,
                                    int band_number, int *widths, int *heights, int max_length);

    int get_crs_wkt(uint64_t token, int dataset, int attempts, int copies,
                    char *crs, int max_size);

    int get_crs_proj4(uint64_t token, int dataset, int attempts, int copies,
                      char *crs, int max_size);

    int get_band_nodata(uint64_t token, int dataset, int attempts, int copies,
                        int band_number, double *nodata, int *success);

    int get_band_min_max(uint64_t token, int dataset, int attempts, int copies,
                         int band_number, int approx_okay, double *minmax, int *success);

    int get_band_data_type(uint64_t token, int dataset, int attempts, int copies,
                           int band_number, int *data_type);

    int get_band_count(uint64_t token, int dataset, int attempts, int copies,
                       int *band_count);

    int get_width_height(uint64_t token, int dataset, int attempts, int copies,
                         int *width, int *height);

    int get_data(uint64_t token, int datasets, int attempts, uint64_t nanos, int copies,
                 int src_window[4],
                 int dst_window[2],
                 int band_number,
                 int type,
                 void *data);

    int get_transform(uint64_t token, int dataset, int attempts, int copies,
                      double transform[6]);

#ifdef __cplusplus
}
#endif

#endif
