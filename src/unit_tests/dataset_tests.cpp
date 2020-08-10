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
#include "errorcodes.hpp"

auto uri1 = uri_t("../experiments/data/c41078a1.tif");
auto options1 = options_t{"-r", "bilinear",
                          "-t_srs", "epsg:3857",
                          "-co", "BLOCKXSIZE=512", "-co", "BLOCKYSIZE=512"};
auto options2 = options_t{"-r", "bilinear",
                          "-t_srs", "epsg:3857",
                          "-dstnodata", "107"};
auto uri_options1 = std::make_pair(uri1, options1);
auto uri_options2 = std::make_pair(uri1, options2);

BOOST_AUTO_TEST_CASE(init)
{
    GDALAllRegister();
}

BOOST_AUTO_TEST_CASE(get_block_size)
{
    auto ld = locked_dataset(uri_options1);
    int width, height;

    errno_init();

    ld.get_block_size(locked_dataset::SOURCE, 1, &width, &height);
    BOOST_TEST(width == 7202);
    BOOST_TEST(height == 1);
    width = height = 0;

    ld.get_block_size(locked_dataset::WARPED, 1, &width, &height);
    BOOST_TEST(width == 512);
    BOOST_TEST(height == 128);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_histogram)
{
    auto ld = locked_dataset(uri_options1);
    errno_init();

    GUIntBig panContainer[256];
    ld.get_histogram(locked_dataset::SOURCE, 1, -0.5, 255.5, 256, *&panContainer, true, false);

    BOOST_TEST(panContainer[0] == 3265829);
    BOOST_TEST(panContainer[12] == 487792);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_offset)
{
    auto ld = locked_dataset(uri_options1);
    double offset = 42;
    int success = false;

    errno_init();

    ld.get_offset(locked_dataset::SOURCE, 1, &offset, &success);
    BOOST_TEST(offset == 0);
    // In GDAL 3.1 there happened a behavior change
    // Both GetScale and GetOffset functions are affected.
    // In the test raster Scale and Offset are not set and in GDAL 2.x and 3.0.x this case is handled differently.
    //
    // The root of the change:
    // Even though Offset and Scale values were not set, there was a logic that fixed it and set values to 1.0 and 0.0 in GDAL 2.x and GDAL 3.0.x.
    // The change in the references changes the success flag behavior, so on the Python side 
    // it is possible to interpret the return results as None.
    //
    // It is easy to check it with the following python code:
    // import gdal
    // path = /tmp/c41078a1.tif
    // ds = gdal.Open(path)
    // band = ds.GetRasterBand(1)
    // s = band.GetScale() # prints 1.0 in GDAL 2.x and 3.0.x, and None in GDAL 3.1.x
    // o = band.GetOffset() # prints 0.0 in GDAL 2.x and 3.0.x, and None in GDAL 3.1.x
    // 
    // However, the Warp operation peformed in this test sets Scale and Offset values explicitly.
    // It explains why the old test case with the Warped dataset didn't change its behavior.
    // 
    // It is easy to check it with the following python code:
    // w = gdal.Warp('', path, format='MEM', warpOptions=["-r", "bilinear", "-t_srs", "epsg:3857", "-co", "BLOCKXSIZE=512", "-co", "BLOCKYSIZE=512"])
    // wband = w.GetRasterBand(1)
    // ws = wband.GetScale() # prints 1.0 in GDAL 2.x, 3.0.x, and GDAL 3.1.x
    // wo = wband.GetOffset() # prints 0.0 in GDAL 2.x, 3.0.x, and GDAL 3.1.x
    //
    // References:
    // - https://github.com/geotrellis/gdal-warp-bindings/pull/96 (PR that introduced this comment)
    // - https://github.com/OSGeo/gdal/issues/2579
    // - https://github.com/OSGeo/gdal/commit/69f25f253d141faf836c400676f9f94dd3f43707
    // 
    // BOOST_TEST(success != false);
    BOOST_TEST(success == false);
    success = false;

    ld.get_offset(locked_dataset::WARPED, 1, &offset, &success);
    BOOST_TEST(offset == 0);
    BOOST_TEST(success != false);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_scale)
{
    auto ld = locked_dataset(uri_options1);
    double scale = 42;
    int success = false;

    errno_init();

    ld.get_scale(locked_dataset::SOURCE, 1, &scale, &success);
    BOOST_TEST(scale == 1);
    // In GDAL 3.1 there happened a behavior change
    // Both GetScale and GetOffset functions are affected.
    // In the test raster Scale and Offset are not set and in GDAL 2.x and 3.0.x this case is handled differently.
    // 
    // The root of the change:
    // Even though Offset and Scale values were not set, there was a logic that fixed it and set values to 1.0 and 0.0 in GDAL 2.x and GDAL 3.0.x.
    // The change in the references changes the success flag behavior, so on the Python side 
    // it is possible to interpret the return results as None.
    // 
    // It is easy to check it with the following python code:
    // import gdal
    // path = /tmp/c41078a1.tif
    // ds = gdal.Open(path)
    // band = ds.GetRasterBand(1)
    // s = band.GetScale() // prints 1.0 in GDAL 2.x and 3.0.x and None in GDAL 3.1.x
    // o = band.GetOffset() // prints 0.0 in GDAL 2.x and 3.0.x and None in GDAL 3.1.x
    // 
    // However, the Warp operation peformed in this test sets Scale and Offset values explicitly.
    // It explains why the old test case with the Warped dataset didn't change its behavior.
    // 
    // It is easy to check it with the following python code:
    // w = gdal.Warp('', path, format='MEM', warpOptions=["-r", "bilinear", "-t_srs", "epsg:3857", "-co", "BLOCKXSIZE=512", "-co", "BLOCKYSIZE=512"])
    // wband = w.GetRasterBand(1)
    // ws = wband.GetScale() // prints 1.0 in GDAL 2.x, 3.0.x and GDAL 3.1.x
    // wo = wband.GetOffset() // prints 0.0 in GDAL 2.x, 3.0.x and GDAL 3.1.x
    //
    // References:
    // - https://github.com/geotrellis/gdal-warp-bindings/pull/96 (PR that introduced this comment)
    // - https://github.com/OSGeo/gdal/issues/2579
    // - https://github.com/OSGeo/gdal/commit/69f25f253d141faf836c400676f9f94dd3f43707
    // 
    // BOOST_TEST(success != false);
    BOOST_TEST(success == false);
    success = false;

    ld.get_scale(locked_dataset::WARPED, 1, &scale, &success);
    BOOST_TEST(scale == 1);
    BOOST_TEST(success != false);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_color_interpretation)
{
    auto ld = locked_dataset(uri_options1);
    int color_interp;

    errno_init();

    ld.get_color_interpretation(locked_dataset::SOURCE, 1, &color_interp);

    BOOST_TEST(color_interp == GCI_PaletteIndex);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_file_metadata_domain_list)
{
    auto ld = locked_dataset(uri_options1);
    char **domain_list = nullptr;

    errno_init();

    ld.get_metadata_domain_list(locked_dataset::SOURCE, 0, &domain_list);

    BOOST_TEST(CSLCount(domain_list) == 3);
    BOOST_TEST(std::string(domain_list[0]) == "");
    BOOST_TEST(std::string(domain_list[1]) == "IMAGE_STRUCTURE");
    BOOST_TEST(std::string(domain_list[2]) == "DERIVED_SUBDATASETS");
    CSLDestroy(domain_list);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_file_metadata)
{
    auto ld = locked_dataset(uri_options1);
    char **list = nullptr;

    errno_init();

    ld.get_metadata(locked_dataset::SOURCE, 0, "", &list);

    BOOST_TEST(CSLCount(list) == 4);
    BOOST_TEST(std::string(CSLFetchNameValue(list, "AREA_OR_POINT")) == "Area");
    BOOST_TEST(std::string(CSLFetchNameValue(list, "TIFFTAG_RESOLUTIONUNIT")) == "2 (pixels/inch)");
    BOOST_TEST(std::string(CSLFetchNameValue(list, "TIFFTAG_XRESOLUTION")) == "72");
    BOOST_TEST(std::string(CSLFetchNameValue(list, "TIFFTAG_YRESOLUTION")) == "72");

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_file_metadata_item)
{
    auto ld = locked_dataset(uri_options1);
    const char *value = nullptr;

    errno_init();

    ld.get_metadata_item(locked_dataset::SOURCE, 0, "AREA_OR_POINT", "", &value);

    BOOST_TEST(std::string(value) == "Area");

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_band_metadata_domain_list)
{
    auto ld = locked_dataset(uri_options1);
    char **domain_list = nullptr;

    errno_init();

    ld.get_metadata_domain_list(locked_dataset::SOURCE, 1, &domain_list);

    BOOST_TEST(CSLCount(domain_list) == 0);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_band_metadata)
{
    auto ld = locked_dataset(uri_options1);
    char **list = nullptr;

    errno_init();

    ld.get_metadata(locked_dataset::SOURCE, 1, "", &list);

    BOOST_TEST(CSLCount(list) == 0);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_band_metadata_item)
{
    auto ld = locked_dataset(uri_options1);
    const char *value = nullptr;

    errno_init();

    ld.get_metadata_item(locked_dataset::SOURCE, 1, "AREA_OR_POINT", "", &value);

    BOOST_TEST(value == nullptr);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(overview_test)
{
    constexpr int N = 3;
    auto ld = locked_dataset(uri_options1);
    int actual_widths[N];
    int actual_heights[N];
    auto actual = std::vector<int>();
    auto expected = std::vector<int>{-1, -1, -1, -1, -1, -1};

    errno_init();

    ld.get_overview_widths_heights(locked_dataset::WARPED, 1, actual_widths, actual_heights, N);
    actual.insert(actual.begin(), actual_widths, actual_widths + 3);
    actual.insert(actual.begin(), actual_heights, actual_heights + 3);

    BOOST_TEST(actual == expected);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_crs_proj4_test)
{
    constexpr int N = 1 << 10;
    auto ld = locked_dataset(uri_options1);
    char actual_chars[N];
    auto expected1 = std::string("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
    auto expected2 = std::string("+proj=merc +a=6378137 +b=6378137 +lat_ts=0 +lon_0=0 +x_0=0 +y_0=0 +k=1 +units=m +nadgrids=@null +wktext +no_defs");

    errno_init();

    ld.get_crs_proj4(locked_dataset::WARPED, actual_chars, N);
    auto actual = std::string(actual_chars);
    boost::trim(actual);
    BOOST_TEST((actual == expected1 || actual == expected2));

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_source_test)
{
    constexpr int N = 1 << 10;
    auto ld = locked_dataset(uri_options1);
    char actual_chars[N];
    auto expected = std::string("+proj=utm +zone=17 +datum=WGS84 +units=m +no_defs");

    errno_init();

    ld.get_crs_proj4(locked_dataset::SOURCE, actual_chars, N);
    auto actual = std::string(actual_chars);
    boost::trim(actual);
    BOOST_TEST(actual == expected);

    int width = -1;
    int height = -1;

    ld.get_width_height(locked_dataset::SOURCE, &width, &height);
    BOOST_TEST(width == 7202);
    BOOST_TEST(height == 5593);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_crs_wkt_test)
{
    constexpr int N = 1 << 10;
    auto ld = locked_dataset(uri_options1);
    char actual_chars[N];
    auto expected1 = std::string("PROJCS[\"WGS 84 / Pseudo-Mercator\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Mercator_1SP\"],PARAMETER[\"central_meridian\",0],PARAMETER[\"scale_factor\",1],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"X\",EAST],AXIS[\"Y\",NORTH],EXTENSION[\"PROJ4\",\"+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs\"],AUTHORITY[\"EPSG\",\"3857\"]]");
    auto expected2 = std::string("PROJCS[\"WGS 84 / Pseudo-Mercator\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Mercator_1SP\"],PARAMETER[\"central_meridian\",0],PARAMETER[\"scale_factor\",1],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],EXTENSION[\"PROJ4\",\"+proj=merc +a=6378137 +b=6378137 +lat_ts=0 +lon_0=0 +x_0=0 +y_0=0 +k=1 +units=m +nadgrids=@null +wktext +no_defs\"],AUTHORITY[\"EPSG\",\"3857\"]]");

    errno_init();

    ld.get_crs_wkt(locked_dataset::WARPED, actual_chars, N);
    auto actual = std::string(actual_chars);
    boost::trim(actual);
    BOOST_TEST((actual == expected1 || actual == expected2));

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_band_count_test)
{
    int band_count;
    auto ld = locked_dataset(uri_options1);

    errno_init();

    ld.get_band_count(locked_dataset::WARPED, &band_count);

    BOOST_TEST(band_count == 1);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_band_data_type)
{
    GDALDataType data_type;
    auto ld = locked_dataset(uri_options1);

    errno_init();

    ld.get_band_data_type(locked_dataset::WARPED, 1, &data_type);

    BOOST_TEST(data_type == GDT_Byte);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_min_max_noapprox)
{
    auto ld = locked_dataset(uri_options1);
    double minmax[2];
    int success;

    errno_init();

    ld.get_band_max_min(locked_dataset::SOURCE, 1, false, minmax, &success);

    BOOST_TEST(success == 0);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_min_max_yesapprox)
{
    auto ld = locked_dataset(uri_options1);
    double minmax[2];
    int success;

    errno_init();

    ld.get_band_max_min(locked_dataset::SOURCE, 1, true, minmax, &success);

    BOOST_TEST(success != 0);
    BOOST_TEST(minmax[0] == 0.0);
    BOOST_TEST(minmax[1] == 12.0);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_band_nodata)
{
    double nodata;
    int success;
    auto ld = locked_dataset(uri_options2);

    errno_init();

    ld.get_band_nodata(locked_dataset::WARPED, 1, &nodata, &success);

    BOOST_TEST(nodata == 107.0);
    BOOST_TEST(success);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_transform_test)
{
    double transform[6];
    auto ld = locked_dataset(uri_options1);
    auto actual = std::vector<double>();
    auto expected = std::vector<double>{
        -8915910.5905594081, 33.88424960091178, 0,
        5174836.3438357478, 0, -33.88424960091178}; // Manually verified

    errno_init();

    ld.get_transform(locked_dataset::WARPED, transform);
    actual.insert(actual.end(), transform, transform + 6);

    BOOST_TEST(actual == expected);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(get_pixels_test)
{
    auto ld = locked_dataset(uri_options1);
    int src_window[4] = {33, 42, 100, 100};
    int dst_window[2] = {4, 2};
    uint64_t actual = 0;
    uint64_t expected = 0x101010001010100; // Manually verified

    errno_init();

    ld.get_pixels(locked_dataset::WARPED, src_window, dst_window, 1, GDT_Byte, &actual);

    BOOST_TEST(actual == expected);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(good_pixels_bad_requests)
{
    auto ld = locked_dataset(uri_options1);
    int src_window[4] = {1000000, 1000000, 500000, 500000};
    int dst_window[2] = {500, 500};
    uint8_t *buffer = new uint8_t[500 * 500];

    errno_init();

    fprintf(stderr, "────────────────────── BEGIN EXPECTED ERROR MESSAGES ─────────────\n");
    auto retval1 = ld.get_pixels(locked_dataset::WARPED, src_window, dst_window, 42, GDT_Byte, buffer);
    auto retval2 = ld.get_pixels(locked_dataset::WARPED, src_window, dst_window, 1, GDT_Byte, buffer);
    auto retval3 = ld.get_pixels(locked_dataset::WARPED, src_window, dst_window, 1, GDT_Byte, nullptr);
    fprintf(stderr, "────────────────────── END EXPECTED ERROR MESSAGES ───────────────\n");
    delete[] buffer;

    errno_deinit();

    BOOST_TEST(retval1 == -CPLE_ObjectNull);
    BOOST_TEST(retval2 == -CPLE_IllegalArg);
    BOOST_TEST(retval3 == -CPLE_AppDefined);
}

BOOST_AUTO_TEST_CASE(move_constructor_test)
{
    auto ld1 = locked_dataset(uri_options1);
    auto ld2 = locked_dataset(std::move(ld1));

    errno_init();

    BOOST_TEST(ld1.valid() == false);
    BOOST_TEST(ld2.valid() == true);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(move_assignment_test)
{
    auto ld1 = locked_dataset(uri_options1);
    auto ld2 = std::move(ld1);

    errno_init();

    BOOST_TEST(ld1.valid() == false);
    BOOST_TEST(ld2.valid() == true);

    errno_deinit();
}

BOOST_AUTO_TEST_CASE(width_height_test)
{
    auto ld = locked_dataset(uri_options1);
    int width = -1;
    int height = -1;

    errno_init();

    ld.get_width_height(locked_dataset::WARPED, &width, &height);
    BOOST_TEST(width == 7319);  // Manually verified (VRT different from raw TIF)
    BOOST_TEST(height == 5771); // Manually verified (VRT different from raw TIF)

    errno_deinit();
}
