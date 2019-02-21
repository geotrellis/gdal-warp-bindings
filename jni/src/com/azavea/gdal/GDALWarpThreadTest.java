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

package com.azavea.gdal;

import java.util.Arrays;
import java.util.Random;

class GDALWarpThreadTest extends Thread {

    private static int WINDOW_SIZE = 1 << 8;
    private static int TILE_SIZE = 1 << 8;
    private static int STEPS = 1 << 12;
    private static int THREADS = 1 << 8;

    private static int[] EXPECTED;

    private static String options[] = { /* */
            "-tap", "-tr", "5", "7", /* */
            "-r", "bilinear", /* */
            "-t_srs", "epsg:3857" /* */
    };

    private int x;
    private int y;
    private long token;

    GDALWarpThreadTest(long _token, int _x, int _y) {
        token = _token;
        x = _x;
        y = _y;
    }

    public void run() {
        int[] src_window = new int[] { -1, -1, WINDOW_SIZE, WINDOW_SIZE };
        int[] dst_window = new int[] { TILE_SIZE, TILE_SIZE };
        byte[] data = new byte[TILE_SIZE * TILE_SIZE];
        Random r = new Random();

        for (int k = 0; k < STEPS; ++k) {
            int i = r.nextInt(x);
            int j = r.nextInt(y);

            src_window[0] = i * WINDOW_SIZE;
            src_window[1] = j * WINDOW_SIZE;

            boolean success = GDALWarp.read_data(token, src_window, dst_window, 1, GDALWarp.GDT_Byte, data);
            int h = data.hashCode();
            assert(success == true);
            assert(h == EXPECTED[i + j * x]);
        }
    }

    public static void main(String[] args) throws InterruptedException {
        GDALWarp.init(127);

        int[] width_height = new int[2];
        int[] src_window = new int[] { -1, -1, WINDOW_SIZE, WINDOW_SIZE };
        int[] dst_window = new int[] { TILE_SIZE, TILE_SIZE };
        byte[] data = new byte[TILE_SIZE * TILE_SIZE];

        long token = GDALWarp.get_token(args[0], options);

        GDALWarp.get_width_height(token, width_height);
        int x = (width_height[0] / WINDOW_SIZE) - 1;
        int y = (width_height[1] / WINDOW_SIZE) - 1;

        EXPECTED = new int[x * y];

        System.out.println("EXPECTED");
        {
            long start = System.currentTimeMillis();
            for (int i = 0; i < x; ++i) {
                for (int j = 0; j < y; ++j) {
                    src_window[0] = i * WINDOW_SIZE;
                    src_window[1] = j * WINDOW_SIZE;
                    boolean success = GDALWarp.read_data(token, src_window, dst_window, 1, GDALWarp.GDT_Byte, data);
                    assert (success == true);
                    EXPECTED[i + j * x] = data.hashCode();
                }
            }
            long end = System.currentTimeMillis();
            System.out.println(end - start);
        }

        System.out.println("ACTUAL");
        {
            GDALWarpThreadTest[] threads = new GDALWarpThreadTest[THREADS];
            long start = System.currentTimeMillis();
            for (int i = 0; i < THREADS; ++i) {
                threads[i] = new GDALWarpThreadTest(token, x, y);
                threads[i].start();
            }
            for (int i = 0; i < THREADS; ++i) {
                threads[i].join();
                System.out.print(".");
            }
            long end = System.currentTimeMillis();
            System.out.println();
            System.out.println(end - start);
        }

        GDALWarp.surrender_token(token);
        GDALWarp.deinit();
        return;
    }
}
