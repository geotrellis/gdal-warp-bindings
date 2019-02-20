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

#define BOOST_TEST_MODULE Locked Dataset Unit Tests
#include <boost/test/included/unit_test.hpp>

#include <vector>

#include <gdal.h>

#include "locked_dataset.hpp"

auto uri = uri_t("experiments/data/c41078a1.tif");
auto options = options_t{"-r", "bilinear", "-t_srs", "epsg:3857"};
auto uri_options = std::make_pair(uri, options);

BOOST_AUTO_TEST_CASE(init)
{
    GDALAllRegister();
}

BOOST_AUTO_TEST_CASE(get_transform_test)
{
    double transform[6];
    auto ld = locked_dataset(uri_options);
    auto actual = std::vector<double>();
    auto expected = std::vector<double>{
        -8915910.5905594081, 33.88424960091178, 0,
        5174836.3438357478, 0, -33.88424960091178}; // Manually verified

    ld.get_transform(transform);
    actual.insert(actual.end(), transform, transform + 6);

    BOOST_TEST(actual == expected);
}

BOOST_AUTO_TEST_CASE(get_pixels_test)
{
    auto ld = locked_dataset(uri_options);
    int src_window[4] = {33, 42, 100, 100};
    int dst_window[2] = {4, 2};
    uint64_t actual = 0;
    uint64_t expected = 0x101010001010100; // Manually verified

    ld.get_pixels(src_window, dst_window, 1, GDT_Byte, &actual);

    BOOST_TEST(actual == expected);
}

BOOST_AUTO_TEST_CASE(copy_constructor_test)
{
    auto ld1 = locked_dataset(uri_options);
    auto ld2 = locked_dataset(ld1);

    BOOST_TEST(ld1.valid() == true);
    BOOST_TEST(ld2.valid() == true);
}

BOOST_AUTO_TEST_CASE(move_constructor_test)
{
    auto ld1 = locked_dataset(uri_options);
    auto ld2 = locked_dataset(std::move(ld1));

    BOOST_TEST(ld1.valid() == false);
    BOOST_TEST(ld2.valid() == true);
}

BOOST_AUTO_TEST_CASE(assignment_test)
{
    auto ld1 = locked_dataset(uri_options);
    auto ld2 = locked_dataset();

    ld2 = ld1;

    BOOST_TEST(ld1.valid() == true);
    BOOST_TEST(ld2.valid() == true);
}

BOOST_AUTO_TEST_CASE(move_assignment_test)
{
    auto ld1 = locked_dataset(uri_options);
    auto ld2 = std::move(ld1);

    BOOST_TEST(ld1.valid() == false);
    BOOST_TEST(ld2.valid() == true);
}

BOOST_AUTO_TEST_CASE(width_height_test)
{
    auto ld = locked_dataset(uri_options);
    int width = -1;
    int height = -1;

    ld.get_width_height(&width, &height);
    BOOST_TEST(width == 7319);
    BOOST_TEST(height == 5771);
}
