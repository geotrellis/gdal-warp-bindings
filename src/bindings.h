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

#ifndef __BINDINGS_H__
#define __BINDINGS_H__

#include <gdal.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void init(int size);
    void deinit();
    uint64_t get_token(const char *uri, const char **options);
    void surrender_token(uint64_t token);
    int read_data(uint64_t token,
                  double src_window[4],
                  int dst_window[2],
                  int band_number,
                  GDALDataType type,
                  void *data);

#ifdef __cplusplus
}
#endif

#endif
