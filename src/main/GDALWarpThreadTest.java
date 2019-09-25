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

import com.azavea.gdal.GDALWarp;
import java.util.Arrays;
import java.util.Random;

class GDALWarpThreadTest extends Thread {

    private static final String ANSI_RESET = "\u001B[0m";
    private static final String ANSI_BLACK = "\u001B[30m";
    private static final String ANSI_RED = "\u001B[31m";
    private static final String ANSI_GREEN = "\u001B[32m";
    private static final String ANSI_YELLOW = "\u001B[33m";
    private static final String ANSI_BLUE = "\u001B[34m";
    private static final String ANSI_PURPLE = "\u001B[35m";
    private static final String ANSI_CYAN = "\u001B[36m";
    private static final String ANSI_WHITE = "\u001B[37m";

    private static int WINDOW_SIZE = 1 << 8;
    private static int TILE_SIZE = 1 << 8;
    private static int STEPS = 1 << 12;
    private static int THREADS = 1 << 8;

    private static int[] EXPECTED;

    private static String options_small[] = { /* */
            "-dstnodata", "107", /* */
            "-tap", "-tr", "33", "42", /* */
            "-r", "bilinear", /* */
            "-t_srs", "epsg:3857", /* */
            "-co", "BLOCKXSIZE=512", "-co", "BLOCKYSIZE=512" /* */
    };

    private static String options_large[] = { /* */
            "-tap", "-tr", "5", "7", /* */
            "-r", "bilinear", /* */
            "-t_srs", "epsg:3857", /* */
            "-co", "BLOCKXSIZE=512", "-co", "BLOCKYSIZE=512" /* */
    };

    private int x;
    private int y;
    private long token;

    /**
     * Constructor.
     *
     * @param _token The token to use
     * @param _x     The width of the dataset
     * @param _y     The height of the dataset
     */
    GDALWarpThreadTest(long _token, int _x, int _y) {
        token = _token;
        x = _x;
        y = _y;
    }

    /**
     * An override of the `run` method (this class derives from Thread).
     */
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

            int return_code = GDALWarp.get_data(token, /* */
                    GDALWarp.WARPED, 0, /* */
                    src_window, dst_window, 1, /* */
                    GDALWarp.GDT_Byte, data);
            boolean success = return_code > 0;
            int h = data.hashCode();
            assert (success == true);
            assert (h == EXPECTED[i + j * x]);
        }
    }

    public static void main(String[] args) throws Exception {
        int[] width_height = new int[2];
        int[] src_window = new int[] { -1, -1, WINDOW_SIZE, WINDOW_SIZE };
        int[] dst_window = new int[] { TILE_SIZE, TILE_SIZE };
        byte[] data = new byte[TILE_SIZE * TILE_SIZE];
        boolean large = true;
        long token;

        if (args.length > 1 && args[1].equals("small")) {
            large = false;
        } else {
            large = true;
        }

        if (large) {
            token = GDALWarp.get_token(args[0], options_large);
        } else {
            token = GDALWarp.get_token(args[0], options_small);
            THREADS = 1 << 4;
        }

        GDALWarp.set_config_option("COMPRESS_OVERVIEW", "DEFLATE"); // Just for fun

        // Version Information
        {
            String version = GDALWarp.get_version_info("--version");
            System.out.println(ANSI_BLUE + "Version info: " + ANSI_GREEN + version + ANSI_RESET);
            System.out.println(ANSI_BLUE + "Version string length: " + ANSI_GREEN + version.length() + ANSI_RESET);
        }

        // License
        {
            String license = GDALWarp.get_version_info("LICENSE");
            System.out.println(ANSI_BLUE + "GDAL license:\n" + ANSI_CYAN + license + "\n" + ANSI_RESET);
            System.out.println(
                    ANSI_BLUE + "GDAL license string length: " + ANSI_CYAN + license.length() + "\n" + ANSI_RESET);
        }

        // Band Count
        {
            int[] band_count = new int[1];
            GDALWarp.get_band_count(token, GDALWarp.WARPED, 0, band_count);
            System.out.println(ANSI_BLUE + "Band Count: " + ANSI_GREEN + band_count[0] + ANSI_RESET);
        }

        // Min, Max
        {
            double[] minmax = new double[2];
            int[] success = new int[1];
            GDALWarp.get_band_min_max(token, GDALWarp.SOURCE, 0, 1, true, minmax, success);
            System.out.println(ANSI_BLUE + "Source Min, Max: " + ANSI_GREEN + minmax[0] + " " + minmax[1] + " ("
                    + success[0] + ")" + ANSI_RESET);
            GDALWarp.get_band_min_max(token, GDALWarp.WARPED, 0, 1, true, minmax, success);
            System.out.println(ANSI_BLUE + "Warped Min, Max: " + ANSI_GREEN + minmax[0] + " " + minmax[1] + " ("
                    + success[0] + ")" + ANSI_RESET);
        }

        // noop
        {
            System.out
                    .println(ANSI_BLUE + "noop: " + ANSI_GREEN + GDALWarp.noop(token, GDALWarp.SOURCE, 0) + ANSI_RESET);
        }

        // Color Interpretation
        {
            int[] color_interp = new int[1];

            GDALWarp.get_color_interpretation(token, GDALWarp.SOURCE, 0, 1, color_interp);
            System.out.println(ANSI_BLUE + "Source color interpretation: " + ANSI_GREEN + color_interp[0] + ANSI_RESET);
            GDALWarp.get_color_interpretation(token, GDALWarp.WARPED, 0, 1, color_interp);
            System.out.println(ANSI_BLUE + "Warped color interpretation: " + ANSI_GREEN + color_interp[0] + ANSI_RESET);
        }

        // Band Type
        {
            int[] data_type = new int[1];
            GDALWarp.get_band_data_type(token, GDALWarp.SOURCE, 0, 1, data_type);
            System.out.println(ANSI_BLUE + "Source data Type: " + ANSI_GREEN + data_type[0] + ANSI_RESET);
            GDALWarp.get_band_data_type(token, GDALWarp.WARPED, 0, 1, data_type);
            System.out.println(ANSI_BLUE + "Warped data Type: " + ANSI_GREEN + data_type[0] + ANSI_RESET);
        }

        // NoData
        {
            double[] nodata = new double[1];
            int[] success = new int[1];
            GDALWarp.get_band_nodata(token, GDALWarp.SOURCE, 0, 1, nodata, success);
            System.out.println(ANSI_BLUE + "Source NoData: " + ANSI_GREEN + nodata[0] + " " + success[0] + ANSI_RESET);
            GDALWarp.get_band_nodata(token, GDALWarp.WARPED, 0, 1, nodata, success);
            System.out.println(ANSI_BLUE + "Warped NoData: " + ANSI_GREEN + nodata[0] + " " + success[0] + ANSI_RESET);
        }

        // Transform
        {
            double[] transform = new double[6];
            GDALWarp.get_transform(token, GDALWarp.SOURCE, 0, transform);
            System.out.print(ANSI_BLUE + "Source Transform:" + ANSI_GREEN);
            for (int i = 0; i < 6; ++i) {
                System.out.print(" " + transform[i]);
            }
            System.out.println(ANSI_RESET);
            GDALWarp.get_transform(token, GDALWarp.WARPED, 0, transform);
            System.out.print(ANSI_BLUE + "Warped Transform:" + ANSI_GREEN);
            for (int i = 0; i < 6; ++i) {
                System.out.print(" " + transform[i]);
            }
            System.out.println(ANSI_RESET);
        }

        // CRS in WKT
        {
            byte[] crs = new byte[1 << 12];
            GDALWarp.get_crs_wkt(token, GDALWarp.SOURCE, 0, crs);
            System.out.println(ANSI_BLUE + "Source WKT CRS: " + ANSI_GREEN + new String(crs, "UTF-8") + ANSI_RESET);
            GDALWarp.get_crs_wkt(token, GDALWarp.WARPED, 0, crs);
            System.out.println(ANSI_BLUE + "Warped WKT CRS: " + ANSI_GREEN + new String(crs, "UTF-8") + ANSI_RESET);
        }

        // CRS in PROJ.4
        {
            byte[] crs = new byte[1 << 12];
            GDALWarp.get_crs_proj4(token, GDALWarp.SOURCE, 0, crs);
            System.out.println(ANSI_BLUE + "Source PROJ.4 CRS: " + ANSI_GREEN + new String(crs, "UTF-8") + ANSI_RESET);
            GDALWarp.get_crs_proj4(token, GDALWarp.WARPED, 0, crs);
            System.out.println(ANSI_BLUE + "Warped PROJ.4 CRS: " + ANSI_GREEN + new String(crs, "UTF-8") + ANSI_RESET);
        }

        // Block Size
        {
            int[] width = new int[1];
            int[] height = new int[1];
            GDALWarp.get_block_size(token, GDALWarp.SOURCE, 0, 1, width, height);
            System.out
                    .println(ANSI_BLUE + "Source block size: " + ANSI_GREEN + width[0] + " " + height[0] + ANSI_RESET);
            GDALWarp.get_block_size(token, GDALWarp.WARPED, 0, 1, width, height);
            System.out
                    .println(ANSI_BLUE + "Warped block size: " + ANSI_GREEN + width[0] + " " + height[0] + ANSI_RESET);
        }

        // Histogram
        {
            long[] histContainer = new long[256];
            GDALWarp.get_histogram(token, GDALWarp.SOURCE, 0, 1, -0.5, 255.5, histContainer, true, false);
            System.out.println(ANSI_BLUE + "Exact histogram: " + ANSI_GREEN);
            for (int i = 0; i < histContainer.length; ++i) {
                System.out.print(histContainer[i] + " ");
            }
            System.out.println("");
        }

        // Offset
        {
            double[] offset = new double[1];
            int[] success = new int[1];
            GDALWarp.get_offset(token, GDALWarp.SOURCE, 0, 1, offset, success);
            System.out.println(ANSI_BLUE + "Source offset: " + ANSI_GREEN + offset[0] + ANSI_RESET);
            GDALWarp.get_offset(token, GDALWarp.WARPED, 0, 1, offset, success);
            System.out.println(ANSI_BLUE + "Warped offset: " + ANSI_GREEN + offset[0] + ANSI_RESET);
        }

        // Scale
        {
            double[] scale = new double[1];
            int[] success = new int[1];
            GDALWarp.get_scale(token, GDALWarp.SOURCE, 0, 1, scale, success);
            System.out.println(ANSI_BLUE + "Source scale: " + ANSI_GREEN + scale[0] + ANSI_RESET);
            GDALWarp.get_scale(token, GDALWarp.WARPED, 0, 1, scale, success);
            System.out.println(ANSI_BLUE + "Warped scale: " + ANSI_GREEN + scale[0] + ANSI_RESET);
        }

        // Domain List Metadata
        {
            byte[][] domain_list1 = new byte[1 << 10][1 << 10];
            System.out.println(ANSI_BLUE + "Source domain list:" + ANSI_RESET);
            GDALWarp.get_metadata_domain_list(token, GDALWarp.SOURCE, 0, 0, domain_list1);
            for (int i = 0; i < domain_list1.length; ++i) {
                String str = new String(domain_list1[i], "UTF-8").trim();
                if (str.length() > 0) {
                    System.out.println(ANSI_GREEN + str + ANSI_RESET);
                }
            }

            byte[][] domain_list2 = new byte[1 << 10][1 << 10];
            System.out.println(ANSI_BLUE + "Warped domain list:" + ANSI_RESET);
            GDALWarp.get_metadata_domain_list(token, GDALWarp.WARPED, 0, 0, domain_list2);
            for (int i = 0; i < domain_list2.length; ++i) {
                String str = new String(domain_list2[i], "UTF-8").trim();
                if (str.length() > 0) {
                    System.out.println(ANSI_GREEN + str + ANSI_RESET);
                }
            }
        }

        // Metadata
        {
            byte[][] list1 = new byte[1 << 10][1 << 10];
            System.out.println(ANSI_BLUE + "Source metadata:" + ANSI_RESET);
            GDALWarp.get_metadata(token, GDALWarp.SOURCE, 0, 0, "", list1);
            for (int i = 0; i < list1.length; ++i) {
                String str = new String(list1[i], "UTF-8").trim();
                if (str.length() > 0) {
                    System.out.println(ANSI_GREEN + str + ANSI_RESET);
                }
            }

            byte[][] list2 = new byte[1 << 10][1 << 10];
            System.out.println(ANSI_BLUE + "Warped metadata:" + ANSI_RESET);
            GDALWarp.get_metadata(token, GDALWarp.WARPED, 0, 0, "", list2);
            for (int i = 0; i < list2.length; ++i) {
                String str = new String(list2[i], "UTF-8").trim();
                if (str.length() > 0) {
                    System.out.println(ANSI_GREEN + str + ANSI_RESET);
                }
            }
        }

        // Metadata item
        {
            byte[] bytes1 = new byte[1 << 10];
            System.out.print(ANSI_BLUE + "Source AREA_OR_POINT:" + ANSI_RESET);
            GDALWarp.get_metadata_item(token, GDALWarp.SOURCE, 0, 0, "AREA_OR_POINT", "", bytes1);
            System.out.println(ANSI_GREEN + new String(bytes1, "UTF-8").trim() + ANSI_RESET);

            byte[] bytes2 = new byte[1 << 10];
            System.out.print(ANSI_BLUE + "Warped AREA_OR_POINT:" + ANSI_RESET);
            GDALWarp.get_metadata_item(token, GDALWarp.WARPED, 0, 0, "AREA_OR_POINT", "", bytes2);
            System.out.println(ANSI_GREEN + new String(bytes1, "UTF-8").trim() + ANSI_RESET);
        }

        // Overviews
        {
            int[] widths = new int[1 << 8];
            int[] heights = new int[1 << 9];
            GDALWarp.get_overview_widths_heights(token, GDALWarp.WARPED, 0, 1, widths, heights);
            int i = 0;
            for (i = 0; i < Math.min(widths.length, heights.length); ++i) {
                if (widths[i] == -1 || heights[i] == -1) {
                    break;
                }
            }
            System.out.println(ANSI_BLUE + "Overviews: " + ANSI_GREEN + i + ANSI_RESET);
        }

        // Dimensions
        GDALWarp.get_width_height(token, GDALWarp.WARPED, 0, width_height);
        System.out.println(
                ANSI_BLUE + "Dimensions: " + ANSI_GREEN + width_height[0] + " " + width_height[1] + ANSI_RESET);
        int x = (width_height[0] / WINDOW_SIZE) - 1;
        int y = (width_height[1] / WINDOW_SIZE) - 1;

        System.out.println();

        EXPECTED = new int[x * y];

        System.out.println(ANSI_YELLOW + "EXPECTED");
        {
            long start = System.currentTimeMillis();
            for (int i = 0; i < x; ++i) {
                for (int j = 0; j < y; ++j) {
                    src_window[0] = i * WINDOW_SIZE;
                    src_window[1] = j * WINDOW_SIZE;
                    int return_code = GDALWarp.get_data(token, /* */
                            GDALWarp.WARPED, 0, /* */
                            src_window, dst_window, 1, /* */
                            GDALWarp.GDT_Byte, data);
                    boolean success = (return_code > 0);
                    assert (success == true);
                    EXPECTED[i + j * x] = data.hashCode();
                }
            }
            long end = System.currentTimeMillis();
            System.out.println("" + (end - start) + ANSI_RESET);
        }

        System.out.println(ANSI_YELLOW + "ACTUAL");
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
            System.out.println("" + (end - start) + ANSI_RESET);
        }

        GDALWarp.deinit();
        return;
    }
}
