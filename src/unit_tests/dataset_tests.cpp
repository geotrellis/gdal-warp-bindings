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
#include <boost/algorithm/string.hpp>

#include <vector>

#include <gdal.h>
#include <gdal_priv.h>

#include "locked_dataset.hpp"

auto uri1 = uri_t("../experiments/data/c41078a1.tif");
auto options1 = options_t{"-r", "bilinear", "-t_srs", "epsg:3857"};
auto options2 = options_t{"-r", "bilinear",
                          "-t_srs", "epsg:3857",
                          "-dstnodata", "107"};
auto uri_options1 = std::make_pair(uri1, options1);
auto uri_options2 = std::make_pair(uri1, options2);

BOOST_AUTO_TEST_CASE(init)
{
    GDALAllRegister();
}

BOOST_AUTO_TEST_CASE(get_file_metadata_domain_list)
{
    auto ld = locked_dataset(uri_options1);
    char **domain_list = nullptr;

    ld.get_metadata_domain_list(locked_dataset::SOURCE, 0, &domain_list);

    BOOST_TEST(CSLCount(domain_list) == 3);
    BOOST_TEST(std::string(domain_list[0]) == "");
    BOOST_TEST(std::string(domain_list[1]) == "IMAGE_STRUCTURE");
    BOOST_TEST(std::string(domain_list[2]) == "DERIVED_SUBDATASETS");
    CSLDestroy(domain_list);
}

BOOST_AUTO_TEST_CASE(get_file_metadata)
{
    auto ld = locked_dataset(uri_options1);
    char **list = nullptr;

    ld.get_metadata(locked_dataset::SOURCE, 0, "", &list);

    BOOST_TEST(CSLCount(list) == 4);
    BOOST_TEST(std::string(CSLFetchNameValue(list, "AREA_OR_POINT")) == "Area");
    BOOST_TEST(std::string(CSLFetchNameValue(list, "TIFFTAG_RESOLUTIONUNIT")) == "2 (pixels/inch)");
    BOOST_TEST(std::string(CSLFetchNameValue(list, "TIFFTAG_XRESOLUTION")) == "72");
    BOOST_TEST(std::string(CSLFetchNameValue(list, "TIFFTAG_YRESOLUTION")) == "72");
}

BOOST_AUTO_TEST_CASE(get_file_metadata_item)
{
    auto ld = locked_dataset(uri_options1);
    const char *value = nullptr;

    ld.get_metadata_item(locked_dataset::SOURCE, 0, "AREA_OR_POINT", "", &value);

    BOOST_TEST(std::string(value) == "Area");
}

BOOST_AUTO_TEST_CASE(get_band_metadata_domain_list)
{
    auto ld = locked_dataset(uri_options1);
    char **domain_list = nullptr;

    ld.get_metadata_domain_list(locked_dataset::SOURCE, 1, &domain_list);

    BOOST_TEST(CSLCount(domain_list) == 0);
}

BOOST_AUTO_TEST_CASE(get_band_metadata)
{
    auto ld = locked_dataset(uri_options1);
    char **list = nullptr;

    ld.get_metadata(locked_dataset::SOURCE, 1, "", &list);

    BOOST_TEST(CSLCount(list) == 0);
}

BOOST_AUTO_TEST_CASE(get_band_metadata_item)
{
    auto ld = locked_dataset(uri_options1);
    const char *value = nullptr;

    ld.get_metadata_item(locked_dataset::SOURCE, 1, "AREA_OR_POINT", "", &value);

    BOOST_TEST(value == nullptr);
}

BOOST_AUTO_TEST_CASE(overview_test)
{
    constexpr int N = 3;
    auto ld = locked_dataset(uri_options1);
    int actual_widths[N];
    int actual_heights[N];
    auto actual = std::vector<int>();
    auto expected = std::vector<int>{-1, -1, -1, -1, -1, -1};

    ld.get_overview_widths_heights(locked_dataset::WARPED, 1, actual_widths, actual_heights, N);
    actual.insert(actual.begin(), actual_widths, actual_widths + 3);
    actual.insert(actual.begin(), actual_heights, actual_heights + 3);

    BOOST_TEST(actual == expected);
}

BOOST_AUTO_TEST_CASE(get_crs_proj4_test)
{
    constexpr int N = 1 << 10;
    auto ld = locked_dataset(uri_options1);
    char actual_chars[N];
    auto expected1 = std::string("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
    auto expected2 = std::string("+proj=merc +a=6378137 +b=6378137 +lat_ts=0 +lon_0=0 +x_0=0 +y_0=0 +k=1 +units=m +nadgrids=@null +wktext +no_defs");

    ld.get_crs_proj4(locked_dataset::WARPED, actual_chars, N);
    auto actual = std::string(actual_chars);
    boost::trim(actual);
    BOOST_TEST((actual == expected1 || actual == expected2));
}

BOOST_AUTO_TEST_CASE(get_source_test)
{
    constexpr int N = 1 << 10;
    auto ld = locked_dataset(uri_options1);
    char actual_chars[N];
    auto expected = std::string("+proj=utm +zone=17 +datum=WGS84 +units=m +no_defs");

    ld.get_crs_proj4(locked_dataset::SOURCE, actual_chars, N);
    auto actual = std::string(actual_chars);
    boost::trim(actual);
    BOOST_TEST(actual == expected);

    int width = -1;
    int height = -1;

    ld.get_width_height(locked_dataset::SOURCE, &width, &height);
    BOOST_TEST(width == 7202);
    BOOST_TEST(height == 5593);
}

BOOST_AUTO_TEST_CASE(get_crs_wkt_test)
{
    constexpr int N = 1 << 10;
    auto ld = locked_dataset(uri_options1);
    char actual_chars[N];
    auto expected1 = std::string("PROJCS[\"WGS 84 / Pseudo-Mercator\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Mercator_1SP\"],PARAMETER[\"central_meridian\",0],PARAMETER[\"scale_factor\",1],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"X\",EAST],AXIS[\"Y\",NORTH],EXTENSION[\"PROJ4\",\"+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs\"],AUTHORITY[\"EPSG\",\"3857\"]]");
    auto expected2 = std::string("PROJCS[\"WGS 84 / Pseudo-Mercator\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Mercator_1SP\"],PARAMETER[\"central_meridian\",0],PARAMETER[\"scale_factor\",1],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],EXTENSION[\"PROJ4\",\"+proj=merc +a=6378137 +b=6378137 +lat_ts=0 +lon_0=0 +x_0=0 +y_0=0 +k=1 +units=m +nadgrids=@null +wktext +no_defs\"],AUTHORITY[\"EPSG\",\"3857\"]]");

    ld.get_crs_wkt(locked_dataset::WARPED, actual_chars, N);
    auto actual = std::string(actual_chars);
    boost::trim(actual);
    BOOST_TEST((actual == expected1 || actual == expected2));
}

BOOST_AUTO_TEST_CASE(get_band_count_test)
{
    int band_count;
    auto ld = locked_dataset(uri_options1);

    ld.get_band_count(locked_dataset::WARPED, &band_count);

    BOOST_TEST(band_count == 1);
}

BOOST_AUTO_TEST_CASE(get_band_data_type)
{
    GDALDataType data_type;
    auto ld = locked_dataset(uri_options1);

    ld.get_band_data_type(locked_dataset::WARPED, 1, &data_type);

    BOOST_TEST(data_type == GDT_Byte);
}

BOOST_AUTO_TEST_CASE(get_min_max_noapprox)
{
    auto ld = locked_dataset(uri_options1);
    double minmax[2];
    int success;

    ld.get_band_max_min(locked_dataset::SOURCE, 1, false, minmax, &success);

    BOOST_TEST(success == 0);
}

BOOST_AUTO_TEST_CASE(get_min_max_yesapprox)
{
    auto ld = locked_dataset(uri_options1);
    double minmax[2];
    int success;

    ld.get_band_max_min(locked_dataset::SOURCE, 1, true, minmax, &success);

    BOOST_TEST(success != 0);
    BOOST_TEST(minmax[0] == 0.0);
    BOOST_TEST(minmax[1] == 12.0);
}

BOOST_AUTO_TEST_CASE(get_band_nodata)
{
    double nodata;
    int success;
    auto ld = locked_dataset(uri_options2);

    ld.get_band_nodata(locked_dataset::WARPED, 1, &nodata, &success);

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

    ld.get_transform(locked_dataset::WARPED, transform);
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

    ld.get_pixels(locked_dataset::WARPED, src_window, dst_window, 1, GDT_Byte, &actual);

    BOOST_TEST(actual == expected);
}

BOOST_AUTO_TEST_CASE(move_constructor_test)
{
    auto ld1 = locked_dataset(uri_options1);
    auto ld2 = locked_dataset(std::move(ld1));

    BOOST_TEST(ld1.valid() == false);
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

    ld.get_width_height(locked_dataset::WARPED, &width, &height);
    BOOST_TEST(width == 7319);  // Manually verified (VRT different from raw TIF)
    BOOST_TEST(height == 5771); // Manually verified (VRT different from raw TIF)
}
