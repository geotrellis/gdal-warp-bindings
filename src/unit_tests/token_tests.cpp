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

#define BOOST_TEST_MODULE Token Unit Tests
#include <boost/test/included/unit_test.hpp>

#include "tokens.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
const char *options1[] = {
    "-of", "MEM",
    "-tap", "-tr", "7", "11",
    "-r", "bilinear",
    "-t_srs", "epsg:3857",
    nullptr};
const char *options2[] = {
    "-of", "MEM",
    "-tap", "-tr", "7", "11",
    "-t_srs", "epsg:3857",
    nullptr};
const char *uri1 = "geo.tif";
const char *uri2 = "geo2.tif";
#pragma GCC diagnostic pop

extern "C"
{
    uint64_t get_token(const char *uri, const char **option1);
    void surrender_token(uint64_t token);
}

BOOST_AUTO_TEST_CASE(get_unique_token_test)
{
    token_init(16);
    auto token = get_token(uri1, options1);

    BOOST_TEST(token == static_cast<token_t>(33));
    token_deinit();
}

BOOST_AUTO_TEST_CASE(get_same_token_test)
{
    token_init(16);
    auto token1 = get_token(uri1, options1);
    auto token2 = get_token(uri1, options1);

    BOOST_TEST(token1 == static_cast<token_t>(33));
    BOOST_TEST(token1 == token2);
    token_deinit();
}

BOOST_AUTO_TEST_CASE(get_different_uri_tokens_test)
{
    token_init(16);
    auto token1 = get_token(uri1, options1);
    auto token2 = get_token(uri2, options1);

    BOOST_TEST(token1 == static_cast<token_t>(33));
    BOOST_TEST(token1 != token2);
    token_deinit();
}

BOOST_AUTO_TEST_CASE(get_different_options_tokens_test)
{
    token_init(16);
    auto token1 = get_token(uri1, options1);
    auto token2 = get_token(uri1, options2);

    BOOST_TEST(token1 == static_cast<token_t>(33));
    BOOST_TEST(token1 != token2);
    token_deinit();
}

BOOST_AUTO_TEST_CASE(different_and_same_test)
{
    token_init(16);
    auto token0 = get_token(uri2, options2);
    auto token1 = get_token(uri1, options1);
    auto token2 = get_token(uri1, options1);
    auto token3 = get_token(uri2, options1);
    auto token4 = get_token(uri2, options1);
    auto token5 = get_token(uri1, options1);

    BOOST_TEST(token1 == token2);
    BOOST_TEST(token2 != token3);
    BOOST_TEST(token3 == token4);
    BOOST_TEST(token4 != token5);
    BOOST_TEST(token5 == token1);
    BOOST_TEST(token0 != token1);
    BOOST_TEST(token0 != token3);
}

BOOST_AUTO_TEST_CASE(token_eviction_test)
{
    token_init(3);
    auto token1 = get_token(uri1, options1);
    auto token2 = get_token(uri1, options2);
    auto token3 = get_token(uri2, options1);
    auto token4 = get_token(uri2, options2);

    BOOST_TEST(query_token(token1).is_initialized() == false);
    BOOST_TEST(query_token(token2).is_initialized() == true);
    BOOST_TEST(query_token(token3).is_initialized() == true);
    BOOST_TEST(query_token(token4).is_initialized() == true);
}

BOOST_AUTO_TEST_CASE(token_lru_eviction_test)
{
    token_init(3);
    auto token1 = get_token(uri1, options1);
    auto token2 = get_token(uri1, options2);
    auto token3 = get_token(uri2, options1);
    query_token(token1);
    auto token4 = get_token(uri2, options2);

    BOOST_TEST(query_token(token1).is_initialized() == true);
    BOOST_TEST(query_token(token2).is_initialized() == false);
    BOOST_TEST(query_token(token3).is_initialized() == true);
    BOOST_TEST(query_token(token4).is_initialized() == true);
}

#if 0
BOOST_AUTO_TEST_CASE(surrender_token_test)
{
    token_init(16);
    auto token1 = get_token(uri1, options1);
    surrender_token(token1);
    auto token2 = get_token(uri2, options1);

    BOOST_TEST(token2 == static_cast<token_t>(33));
    token_deinit();
}

BOOST_AUTO_TEST_CASE(double_free_test)
{
    token_init(16);
    auto token = get_token(uri1, options1);
    surrender_token(token);
    surrender_token(token);

    token_deinit();
}
#endif

namespace std
{
std::ostream &operator<<(std::ostream &out, const uri_options_t &rhs)
{
    return (out << rhs.first << " " << rhs.second.size());
}
} // namespace std

BOOST_AUTO_TEST_CASE(query_test)
{
    token_init(16);

    auto token1 = static_cast<uint64_t>(42);
    auto actual1 = query_token(token1).is_initialized();
    auto expected1 = false;
    BOOST_TEST(actual1 == expected1);

    auto token2 = get_token(uri1, options1);
    auto actual2 = query_token(token2).value();
    auto expected2 = uri_options_t{uri_t{uri1}, options_t{}};
    for (auto p = options1; *p != nullptr; ++p)
    {
        expected2.second.push_back(*p);
    }
    BOOST_TEST(actual2 == expected2);

    auto token3 = get_token(uri1, options2);
    auto actual3 = query_token(token3).value();
    auto expected3 = std::make_pair(uri_t{uri1}, options_t{});
    for (auto p = options2; *p != nullptr; ++p)
    {
        expected3.second.push_back(*p);
    }
    BOOST_TEST(actual3 == expected3);

    token_deinit();
}
