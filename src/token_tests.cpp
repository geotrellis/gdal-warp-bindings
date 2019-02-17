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
    token_init();
    auto token = get_token(uri1, options1);

    BOOST_TEST(token == static_cast<token_t>(33));
    token_deinit();
}

BOOST_AUTO_TEST_CASE(get_same_token_test)
{
    token_init();
    auto token1 = get_token(uri1, options1);
    auto token2 = get_token(uri1, options1);

    BOOST_TEST(token1 == static_cast<token_t>(33));
    BOOST_TEST(token1 == token2);
    token_deinit();
}

BOOST_AUTO_TEST_CASE(get_different_uri_tokens_test)
{
    token_init();
    auto token1 = get_token(uri1, options1);
    auto token2 = get_token(uri2, options1);

    BOOST_TEST(token1 == static_cast<token_t>(33));
    BOOST_TEST(token1 != token2);
    token_deinit();
}

BOOST_AUTO_TEST_CASE(get_different_options_tokens_test)
{
    token_init();
    auto token1 = get_token(uri1, options1);
    auto token2 = get_token(uri1, options2);

    BOOST_TEST(token1 == static_cast<token_t>(33));
    BOOST_TEST(token1 != token2);
    token_deinit();
}

BOOST_AUTO_TEST_CASE(surrender_token_test)
{
    token_init();
    auto token1 = get_token(uri1, options1);
    surrender_token(token1);
    auto token2 = get_token(uri2, options1);

    BOOST_TEST(token2 == static_cast<token_t>(33));
    token_deinit();
}

BOOST_AUTO_TEST_CASE(double_free_test)
{
    token_init();
    auto token = get_token(uri1, options1);
    surrender_token(token);
    surrender_token(token);

    token_deinit();
}
