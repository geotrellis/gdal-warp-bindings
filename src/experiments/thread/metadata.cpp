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

#include <cstdint>
#include <cstdlib>
#include <cassert>

#include <boost/timer/timer.hpp>

#include <gdal.h>

#include "../../bindings.h"
#include "../../locked_dataset.hpp"

char const *options[] = {
    "-dstnodata", "107",
    "-tap", "-tr", "33", "42",
    "-r", "bilinear",
    "-t_srs", "epsg:3857",
    "-co", "BLOCKXSIZE=512", "-co", "BLOCKYSIZE=512",
    nullptr};

namespace t = boost::timer;

void CSLDestroy(char **papszStrList)
{
  if (!papszStrList)
    return;

  for (char **papszPtr = papszStrList; *papszPtr != nullptr; ++papszPtr)
  {
    free(*papszPtr);
  }

  free(papszStrList);
}

int main(int argc, char **argv)
{
  init(33);

  uint64_t token = get_token(argv[1], options);
  auto handle = GDALOpen(argv[1], GA_ReadOnly);

  char **domain_list = NULL;

  {
    t::auto_cpu_timer timer;

    for (int i = 0; i < (1 << 18); ++i)
    {
      assert(noop(token, locked_dataset::SOURCE, 0, 1) > 0);
    }
  }

  {
    t::auto_cpu_timer timer;

    for (int i = 0; i < (1 << 18); ++i)
    {
      assert(get_metadata_domain_list(token, 0, 10, 1, 0, &domain_list) > 0);
      CSLDestroy(domain_list);
    }
  }
  {
    t::auto_cpu_timer timer;

    for (int i = 0; i < (1 << 18); ++i)
    {
      domain_list = GDALGetMetadataDomainList(handle);
      CSLDestroy(domain_list);
    }
  }

  GDALClose(handle);
  deinit();
}
