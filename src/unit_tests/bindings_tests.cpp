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

#define BOOST_TEST_MODULE Bindings Unit Tests
#include <boost/test/included/unit_test.hpp>

#include <cpl_error.h>

#include "bindings.h"
#include "locked_dataset.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
const char *options[] = {
    "-tap", "-tr", "7", "11",
    "-r", "bilinear",
    "-t_srs", "epsg:3857",
    "-dstnodata", "107",
    nullptr};
const char *good_uri = "../experiments/data/c41078a1.tif";
const char *bad_uri = "HOPEFULLY_THERE_IS_NO_FILE_WITH_THIS_NAME.tif";
#pragma GCC diagnostic pop

constexpr int copies = -4;

BOOST_AUTO_TEST_CASE(initialization)
{
    init(1 << 8);
    deinit();
}

BOOST_AUTO_TEST_CASE(good_uri_finite_attempts_example)
{
    init(1 << 8);

    auto token = get_token(good_uri, options);
    double nodata;
    int success;

    BOOST_TEST(get_band_nodata(token, locked_dataset::SOURCE, 42, copies, 1, &nodata, &success) > 0);
    BOOST_TEST(success == 0);

    BOOST_TEST(get_band_nodata(token, locked_dataset::WARPED, 42, copies, 1, &nodata, &success) > 0);
    BOOST_TEST(success != 0);
    BOOST_TEST(nodata == 107.0);

    deinit();
}

BOOST_AUTO_TEST_CASE(good_uri_infinite_attempts_example)
{
    init(1 << 8);

    auto token = get_token(good_uri, options);
    double nodata;
    int success;

    BOOST_TEST(get_band_nodata(token, locked_dataset::SOURCE, 0, copies, 1, &nodata, &success) > 0);
    BOOST_TEST(success == 0);

    BOOST_TEST(get_band_nodata(token, locked_dataset::WARPED, 0, copies, 1, &nodata, &success) > 0);
    BOOST_TEST(success != 0);
    BOOST_TEST(nodata == 107.0);

    deinit();
}

BOOST_AUTO_TEST_CASE(bad_uri_finite_attempts_example)
{
    init(1 << 8);

    auto token = get_token(bad_uri, options);
    double nodata;
    int success;

    BOOST_TEST(get_band_nodata(token, locked_dataset::SOURCE, 3, copies, 1, &nodata, &success) == -CPLE_AppDefined);
    BOOST_TEST(get_band_nodata(token, locked_dataset::WARPED, 4, copies, 1, &nodata, &success) == -CPLE_AppDefined);

    deinit();
}

BOOST_AUTO_TEST_CASE(bad_uri_infinite_attempts_example)
{
    init(1 << 8);

    auto token = get_token(bad_uri, options);
    double nodata;
    int success;

    BOOST_TEST(get_band_nodata(token, locked_dataset::SOURCE, 0, copies, 1, &nodata, &success) == -CPLE_AppDefined);
    BOOST_TEST(get_band_nodata(token, locked_dataset::WARPED, 0, copies, 1, &nodata, &success) == -CPLE_AppDefined);

    deinit();
}

BOOST_AUTO_TEST_CASE(bad_token_finite_attempts_example)
{
    init(1 << 8);

    uint64_t token = 93;
    double nodata;
    int success;

    BOOST_TEST(get_band_nodata(token, locked_dataset::SOURCE, 42, copies, 1, &nodata, &success) == -CPLE_OpenFailed);
    BOOST_TEST(get_band_nodata(token, locked_dataset::WARPED, 42, copies, 1, &nodata, &success) == -CPLE_OpenFailed);

    deinit();
}

BOOST_AUTO_TEST_CASE(bad_token_infinite_attempts_example)
{
    init(1 << 8);

    uint64_t token = 93;
    double nodata;
    int success;

    BOOST_TEST(get_band_nodata(token, locked_dataset::SOURCE, 0, copies, 1, &nodata, &success) == -CPLE_OpenFailed);
    BOOST_TEST(get_band_nodata(token, locked_dataset::WARPED, 0, copies, 1, &nodata, &success) == -CPLE_OpenFailed);

    deinit();
}
