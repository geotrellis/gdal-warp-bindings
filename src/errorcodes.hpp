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

#ifndef __ERRORCODES_H__
#define __ERRORCODES_H__

#include <chrono>
#include <gdal.h>

// warpbindings error codes should not intersect GDAL error codes
// see https://gdal.org/doxygen/cpl__error_8h.html
#define ATTEMPTS_EXCEEDED 100

void errno_init();
void errno_deinit();
void put_last_errno(CPLErr eErrClass, int err_no, const char *msg);
int get_last_errno();
std::chrono::milliseconds get_last_errno_timestamp();

#endif
