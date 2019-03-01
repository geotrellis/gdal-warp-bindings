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

auto uri = uri_t("../experiments/data/c41078a1.tif");
auto options1 = options_t{"-r", "bilinear", "-t_srs", "epsg:3857"};
auto options2 = options_t{"-r", "bilinear",
                          "-t_srs", "epsg:3857",
                          "-dstnodata", "107"};
auto uri_options1 = std::make_pair(uri, options1);
auto uri_options2 = std::make_pair(uri, options2);

BOOST_AUTO_TEST_CASE(init)
{
    GDALAllRegister();
}

BOOST_AUTO_TEST_CASE(overview_test)
{
    constexpr int N = 3;
    auto ld = locked_dataset(uri_options1);
    int actual_widths[N];
    int actual_heights[N];
    auto actual = std::vector<int>();
    auto expected = std::vector<int>{-1, -1, -1, -1, -1, -1};

    ld.get_overview_widths_heights(actual_widths, actual_heights, N);
    actual.insert(actual.begin(), actual_widths, actual_widths + 3);
    actual.insert(actual.begin(), actual_heights, actual_heights + 3);

    BOOST_TEST(actual == expected);
}

BOOST_AUTO_TEST_CASE(get_crs_proj4_test)
{
    constexpr int N = 1 << 10;
    auto ld = locked_dataset(uri_options1);
    char actual[N];
    auto expected = "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs";

    ld.get_crs_proj4(actual, N);

    BOOST_TEST(std::string(actual) == expected);
}

BOOST_AUTO_TEST_CASE(get_crs_wkt_test)
{
    constexpr int N = 1 << 10;
    auto ld = locked_dataset(uri_options1);
    char actual[N];
    auto expected = "PROJCS[\"WGS 84 / Pseudo-Mercator\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Mercator_1SP\"],PARAMETER[\"central_meridian\",0],PARAMETER[\"scale_factor\",1],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"X\",EAST],AXIS[\"Y\",NORTH],EXTENSION[\"PROJ4\",\"+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs\"],AUTHORITY[\"EPSG\",\"3857\"]]";

    ld.get_crs_wkt(actual, N);

    BOOST_TEST(std::string(actual) == expected);
}

BOOST_AUTO_TEST_CASE(get_band_count_test)
{
    int band_count;
    auto ld = locked_dataset(uri_options1);

    ld.get_band_count(&band_count);

    BOOST_TEST(band_count == 1);
}

BOOST_AUTO_TEST_CASE(get_band_data_type)
{
    GDALDataType data_type;
    auto ld = locked_dataset(uri_options1);

    ld.get_band_data_type(1, &data_type);

    BOOST_TEST(data_type == GDT_Byte);
}

BOOST_AUTO_TEST_CASE(get_band_nodata)
{
    double nodata;
    int success;
    auto ld = locked_dataset(uri_options2);

    ld.get_band_nodata(1, &nodata, &success);

    BOOST_TEST(nodata == 107.0);
    BOOST_TEST(success);
}

BOOST_AUTO_TEST_CASE(get_transform_test)
{
    double transform[6];
    auto ld = locked_dataset(uri_options1);
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
    auto ld = locked_dataset(uri_options1);
    int src_window[4] = {33, 42, 100, 100};
    int dst_window[2] = {4, 2};
    uint64_t actual = 0;
    uint64_t expected = 0x101010001010100; // Manually verified

    ld.get_pixels(src_window, dst_window, 1, GDT_Byte, &actual);

    BOOST_TEST(actual == expected);
}

BOOST_AUTO_TEST_CASE(copy_constructor_test)
{
    auto ld1 = locked_dataset(uri_options1);
    auto ld2 = locked_dataset(ld1);

    BOOST_TEST(ld1.valid() == true);
    BOOST_TEST(ld2.valid() == true);
}

BOOST_AUTO_TEST_CASE(move_constructor_test)
{
    auto ld1 = locked_dataset(uri_options1);
    auto ld2 = locked_dataset(std::move(ld1));

    BOOST_TEST(ld1.valid() == false);
    BOOST_TEST(ld2.valid() == true);
}

BOOST_AUTO_TEST_CASE(assignment_test)
{
    auto ld1 = locked_dataset(uri_options1);
    auto ld2 = locked_dataset();

    ld2 = ld1;

    BOOST_TEST(ld1.valid() == true);
    BOOST_TEST(ld2.valid() == true);
}

BOOST_AUTO_TEST_CASE(move_assignment_test)
{
    auto ld1 = locked_dataset(uri_options1);
    auto ld2 = std::move(ld1);

    BOOST_TEST(ld1.valid() == false);
    BOOST_TEST(ld2.valid() == true);
}

BOOST_AUTO_TEST_CASE(width_height_test)
{
    auto ld = locked_dataset(uri_options1);
    int width = -1;
    int height = -1;

    ld.get_width_height(&width, &height);
    BOOST_TEST(width == 7319);  // Manually verified (VRT different from raw TIF)
    BOOST_TEST(height == 5771); // Manually verified (VRT different from raw TIF)
}
