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

#define BOOST_TEST_MODULE Cache Unit Tests
#include <boost/test/included/unit_test.hpp>

#include <gdal.h>

#include "lru_cache.hpp"

auto uri1 = uri_t("../experiments/data/c41078a1.tif");
auto options1 = options_t{
    "-of", "MEM",
    "-tap", "-tr", "7", "11",
    "-r", "bilinear",
    "-t_srs", "epsg:3857"};
auto options2 = options_t{
    "-of", "MEM",
    "-tap", "-tr", "33", "42",
    "-t_srs", "epsg:3857"};
auto options3 = options_t{
    "-of", "MEM",
    "-tap", "-tr", "1013", "1307",
    "-t_srs", "epsg:3857"};
auto uri_options1 = uri_options_t{uri1, options1};
auto uri_options2 = uri_options_t{uri1, options2};
auto uri_options3 = uri_options_t{uri1, options3};

namespace std
{
std::ostream &operator<<(std::ostream &out, const uri_options_t &rhs)
{
    return (out << "uri_options_t");
}

std::ostream &operator<<(std::ostream &out, const locked_dataset &rhs)
{
    return (out << "locked_dataset_t");
}
} // namespace std

BOOST_AUTO_TEST_CASE(init)
{
    GDALAllRegister();
}

BOOST_AUTO_TEST_CASE(get_capacity_test)
{
    auto cache = lru_cache(33);
    BOOST_TEST(cache.capacity() == 33);
}

BOOST_AUTO_TEST_CASE(get_same_test)
{
    auto cache = lru_cache(4);
    BOOST_TEST(cache.size() == 0);
    cache.get(uri_options1);
    BOOST_TEST(cache.size() == 1);
    cache.get(uri_options1);
    BOOST_TEST(cache.size() == 1);
    cache.clear();
    BOOST_TEST(cache.size() == 0);
}

BOOST_AUTO_TEST_CASE(get_different_test)
{
    auto cache = lru_cache(4);
    BOOST_TEST(cache.size() == 0);
    cache.get(uri_options1);
    BOOST_TEST(cache.size() == 1);
    cache.get(uri_options2);
    BOOST_TEST(cache.size() == 2);
    cache.clear();
    BOOST_TEST(cache.size() == 0);
}

BOOST_AUTO_TEST_CASE(enforce_capacity_limit_test)
{
    auto cache = lru_cache(1);
    cache.get(uri_options1);
    cache.get(uri_options2);
    BOOST_TEST(cache.size() == 1);
    BOOST_TEST(cache.count(uri_options1) == 1);
    BOOST_TEST(cache.count(uri_options2) == 0);
}

BOOST_AUTO_TEST_CASE(evict_unused_test)
{
    auto cache = lru_cache(1);
    auto v = cache.get(uri_options1);
    v[0]->dec();
    cache.get(uri_options2);
    BOOST_TEST(cache.size() == 1);
    BOOST_TEST(cache.count(uri_options1) == 0);
    BOOST_TEST(cache.count(uri_options2) == 1);
}

BOOST_AUTO_TEST_CASE(evict_correct_test)
{
    auto cache = lru_cache(2);
    auto v = cache.get(uri_options1);
    v[0]->dec();
    v = cache.get(uri_options2);
    v[0]->dec();
    cache.get(uri_options3);
    BOOST_TEST(cache.size() == 2);
    BOOST_TEST(cache.count(uri_options1) == 0);
    BOOST_TEST(cache.count(uri_options2) == 1);
    BOOST_TEST(cache.count(uri_options3) == 1);
}

BOOST_AUTO_TEST_CASE(non_destructive_get_test)
{
    auto cache = lru_cache(4);
    auto v1 = cache.get(uri_options1);
    auto v2 = cache.get(uri_options1);
    auto v3 = cache.get(uri_options1);
    auto v4 = cache.get(uri_options1);
    BOOST_TEST(v1[0] == v2[0]);
    BOOST_TEST(v2[0] == v3[0]);
    BOOST_TEST(v3[0] == v4[0]);
    BOOST_TEST(cache.size() == 1);
}
