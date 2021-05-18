/*
 * Copyright 2019-2021 Azavea
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

import java.io.UnsupportedEncodingException;

import cz.adamh.utils.NativeUtils;

public class GDALWarp {
        private static final String GDALWARP_BINDINGS_RESOURCE_ARM64_ELF = "/resources/libgdalwarp_bindings-arm64.so";
        private static final String GDALWARP_BINDINGS_RESOURCE_AMD64_ELF = "/resources/libgdalwarp_bindings-amd64.so";
        private static final String GDALWARP_BINDINGS_RESOURCE_AMD64_MACHO = "/resources/libgdalwarp_bindings-amd64.dylib";
        private static final String GDALWARP_BINDINGS_RESOURCE_AMD64_DLL = "/resources/gdalwarp_bindings-amd64.dll";

        public static final int GDT_Unknown = 0;
        public static final int GDT_Byte = 1;
        public static final int GDT_UInt16 = 2;
        public static final int GDT_Int16 = 3;
        public static final int GDT_UInt32 = 4;
        public static final int GDT_Int32 = 5;
        public static final int GDT_Float32 = 6;
        public static final int GDT_Float64 = 7;
        public static final int GDT_CInt16 = 8;
        public static final int GDT_CInt32 = 9;
        public static final int GDT_CFloat32 = 10;
        public static final int GDT_CFloat64 = 11;
        public static final int GDT_TypeCount = 12;

        public static final int GCI_Undefined = 0;
        public static final int GCI_GrayIndex = 1;
        public static final int GCI_PaletteIndex = 2;
        public static final int GCI_RedBand = 3;
        public static final int GCI_GreenBand = 4;
        public static final int GCI_BlueBand = 5;
        public static final int GCI_AlphaBand = 6;
        public static final int GCI_HueBand = 7;
        public static final int GCI_SaturationBand = 8;
        public static final int GCI_LightnessBand = 9;
        public static final int GCI_CyanBand = 10;
        public static final int GCI_MagentaBand = 11;
        public static final int GCI_YellowBand = 12;
        public static final int GCI_BlackBand = 13;
        public static final int GCI_YCbCr_YBand = 14;
        public static final int GCI_YCbCr_CbBand = 15;
        public static final int GCI_YCbCr_CrBand = 16;
        public static final int GCI_Max = 16;

        public static final int SOURCE = 0;
        public static final int WARPED = 1;

        private static final String ANSI_RESET = "\u001B[0m";
        private static final String ANSI_RED = "\u001B[31m";

        private static ThreadLocal<byte[]> scratch_1d = new ThreadLocal<>();
        private static ThreadLocal<byte[][]> scratch_2d = new ThreadLocal<>();

        private static native void _init(int size);

        private static int ensure_scratch_1d() {
                int len;

                if (scratch_1d.get() == null) {
                        len = 1 << 8;
                        scratch_1d.set(new byte[len]);
                } else {
                        len = scratch_1d.get().length;
                }
                return len;
        }

        private static int ensure_scratch_2d() {
                int len;

                if (scratch_2d.get() == null) {
                        len = 1 << 8;
                        scratch_2d.set(new byte[len][len]);
                } else {
                        len = scratch_2d.get().length;
                }
                return len;
        }

        private static void grow_scratch_1d(int _len) {
                int len;

                if (_len <= 0) {
                        len = (scratch_1d.get().length << 1);
                } else {
                        len = _len;
                }
                scratch_1d.set(new byte[len]);
        }

        private static void grow_scratch_2d(int _len) {
                int len;

                if (_len <= 0) {
                        len = (scratch_2d.get().length << 1);
                } else {
                        len = _len;
                }
                scratch_2d.set(new byte[len][len]);
        }

        public static void init(int size) throws Exception {

                String os = System.getProperty("os.name").toLowerCase();
                String arch = System.getProperty("os.arch").toLowerCase();

                // Try to load ELF shared object from JAR file ...
                if (os.contains("linux")) {
                        // AMD64
                        if (arch.contains("amd64"))
                        {
                                NativeUtils.loadLibraryFromJar(GDALWARP_BINDINGS_RESOURCE_AMD64_ELF);
                        }
                        // ARM64
                        else if (arch.contains("aarch64"))
                        {
                                NativeUtils.loadLibraryFromJar(GDALWARP_BINDINGS_RESOURCE_ARM64_ELF);
                        }
                }
                // Try to Load Mach-O shared object from JAR file ...
                else if (os.contains("mac")) {
                        NativeUtils.loadLibraryFromJar(GDALWARP_BINDINGS_RESOURCE_AMD64_MACHO);
                }
                // Try to load Windows DLL from JAR file ...
                else if (os.contains("win")) {
                        NativeUtils.loadLibraryFromJar(GDALWARP_BINDINGS_RESOURCE_AMD64_DLL);
                } else {
                        throw new Exception("Unsupported platform");
                }

                _init(size);
        }

        static {
                try {
                        init(1 << 8);
                } catch (Exception e) {
                        System.err.println(ANSI_RED + "INITIALIZATION FAILED" + ANSI_RESET);
                }
        }

        /**
         * Get GDAL version information.
         *
         * @param key The key of the desired information (please see
         *            https://gdal.org/api/raster_c_api.html?highlight=rasterio#_CPPv415GDALVersionInfoPKc)
         * @return The requested information
         */
        public static String get_version_info(String key) throws UnsupportedEncodingException {
                int array_len = ensure_scratch_1d();
                int result_len = _get_version_info(key, scratch_1d.get());

                if (result_len < array_len) {
                        return new String(scratch_1d.get(), 0, result_len, "UTF-8").trim();
                } else {
                        grow_scratch_1d(result_len + 1);
                        _get_version_info(key, scratch_1d.get());
                        return new String(scratch_1d.get(), 0, result_len, "UTF-8").trim();
                }
        }

        /**
         * The deinitialization function for the library.
         */
        public static native void deinit();

        public static native int _get_version_info(String key, byte[] value);

        /**
         * Set a GDAL configuration key, value pair.
         *
         * @param key   The key to set
         * @param value The vale to associate with the key
         */
        public static native void set_config_option(String key, String value);

        /**
         * Return an unused token for the given uri, options pair. This is not a
         * function: two subsequent calls to it with the same pair as input may produce
         * different output.
         *
         * @param uri     A string containing the URI
         * @param options An array of strings contains the warp options
         * @return A token that was not in use prior to the call
         */
        public static native long get_token(String uri, String[] options);

        /**
         * Get the block size of the given band.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band_number The band in question
         * @param width       The return-location of the block width
         * @param height      The return-location of the block height
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_block_size(long token, int dataset, int attempts, /* */
                        int band_number, int[] width, int[] height);

        /**
         * Get the histogram for a given band
         *
         * @param token                A token associated with some uri, options pair
         * @param dataset              0 (or GDALWarp::SOURCE) for the source dataset, 1
         *                             (or GDALWarp::WARPED) for the warped dataset
         * @param attempts             The number of attempts to make before giving up
         * @param band_number          The band in question
         * @param min                  The minimum bin for the histogram
         * @param max                  The maximum bin for the histogram
         * @param histogram_container  The array to hold the result of the histogram
         *                             calculation
         * @param include_out_of_range Whether to map out of range values into the
         *                             first/last buckets
         * @param approx_ok            Whether to accept an approximate histogram. With
         *                             COGs, will cause the use of overviews
         */
        public static native int get_histogram(long token, int dataset, int attempts, /* */
                        int band_number, double min, double max, long[] histogram_container,
                        boolean include_out_of_range, boolean approx_ok);

        /**
         * Get the offset of the given band.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band_number The band in question
         * @param offset      The return-location of the offset
         * @param success     The return-location of the success flag
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_offset(long token, int dataset, int attempts, /* */
                        int band_number, double[] offset, int[] success);

        /**
         * Get the scale of the given band.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band_number The band in question
         * @param scale       The return-location of the scale
         * @param success     The return-location of the success flag
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_scale(long token, int dataset, int attempts, /* */
                        int band_number, double[] scale, int[] success);

        /**
         * Get the color interpretation of the given band.
         *
         * @param token    A token associated with some uri, options pair
         * @param dataset  0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                 GDALWarp::WARPED) for the warped dataset
         * @param attempts The number of attempts to make before giving up
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int noop(long token, int dataset, int attempts);

        /**
         * Get the color interpretation of the given band.
         *
         * @param token        A token associated with some uri, options pair
         * @param dataset      0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                     GDALWarp::WARPED) for the warped dataset
         * @param attempts     The number of attempts to make before giving up
         * @param band_number  The band in question
         * @param color_interp The return-slot for the integer-coded color
         *                     interpretation
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_color_interpretation(long token, int dataset, int attempts, /* */
                        int band_number, int color_interp[]);

        /**
         * Get the list of metadata domain lists.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band_number The band to query (zero for the file itself)
         * @param domain_list The return-location for the list of strings
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_metadata_domain_list(long token, int dataset, int attempts, /* */
                        int band_number, byte[][] domain_list);

        /**
         * Get the list of metadata domain lists.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band_number The band to query (zero for the file itself)
         * @param domain_list The return-location for the list of strings
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static int get_metadata_domain_list(long token, int dataset, int attempts, int band_number,
                        String[][] domain_list) throws UnsupportedEncodingException {
                int array_len = ensure_scratch_2d();
                int retval = get_metadata_domain_list(token, dataset, attempts, band_number, scratch_2d.get());

                if (retval >= 0) {
                        int n = 0;
                        for (n = 0; scratch_2d.get()[n][0] != 0; ++n) {
                        }
                        domain_list[0] = new String[n];
                        for (--n; n >= 0; --n) {
                                int m = 0;
                                byte[] bytes = scratch_2d.get()[n];
                                for (m = 0; (m < array_len) && (bytes[m] != 0); ++m) {
                                }
                                domain_list[0][n] = new String(bytes, 0, m, "UTF-8").trim();
                        }
                        return retval;
                } else if (retval == -1) { // CPLE_AppDefined means array too small
                        grow_scratch_2d(0);
                        return get_metadata_domain_list(token, dataset, attempts, band_number, domain_list);
                } else { // Return other error code
                        return retval;
                }
        }

        /**
         * Get the metadata found in a particular metadata domain.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band_number The band to query (zero for the file itself)
         * @param domain      The metadata domain to query
         * @param list        The return-location for the list of strings
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_metadata(long token, int dataset, int attempts, /* */
                        int band_number, String domain, byte[][] list);

        /**
         * Get the metadata found in a particular metadata domain.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band_number The band to query (zero for the file itself)
         * @param domain      The metadata domain to query
         * @param list        The return-location for the list of strings
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static int get_metadata(long token, int dataset, int attempts, int band_number, String domain,
                        String[][] list) throws UnsupportedEncodingException {
                int array_len = ensure_scratch_2d();
                int retval = get_metadata(token, dataset, attempts, band_number, domain, scratch_2d.get());

                if (retval >= 0) {
                        int n = 0;
                        for (n = 0; scratch_2d.get()[n][0] != 0; ++n) {
                        }
                        list[0] = new String[n];
                        for (--n; n >= 0; --n) {
                                int m = 0;
                                byte[] bytes = scratch_2d.get()[n];
                                for (m = 0; (m < array_len) && (bytes[m] != 0); ++m) {
                                }
                                list[0][n] = new String(bytes, 0, m, "UTF-8").trim();
                        }
                        return retval;
                } else if (retval == -1) { // CPLE_AppDefined means array too small
                        grow_scratch_2d(0);
                        return get_metadata(token, dataset, attempts, band_number, domain, list);
                } else { // Return other error code
                        return retval;
                }
        }

        /**
         * Get a particular metadata value associated with a key.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band_number The band to query (zero for the file itself)
         * @param key         The key of the key, value metadata pair
         * @param domain      The metadata domain to query
         * @param value       The return-location for the value of the key, value pair
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_metadata_item(long token, int dataset, int attempts, /* */
                        int band_number, String key, String domain, byte[] value);

        /**
         * Get a particular metadata value associated with a key.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band_number The band to query (zero for the file itself)
         * @param key         The key of the key, value metadata pair
         * @param domain      The metadata domain to query
         * @param value       The return-location for the value of the key, value pair
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static int get_metadata_item(long token, int dataset, int attempts, int band_number, String key,
                        String domain, String[] value) throws UnsupportedEncodingException {
                int array_len = ensure_scratch_1d();
                int retval = get_metadata_item(token, dataset, attempts, band_number, key, domain, scratch_1d.get());

                if (retval >= 0) {
                        int m = 0;
                        byte[] bytes = scratch_1d.get();
                        for (m = 0; (m < array_len) && (bytes[m] != 0); ++m) {
                        }
                        value[0] = new String(bytes, 0, m, "UTF-8").trim();
                        return retval;
                } else if (retval == -1) { // CPLE_AppDefined means array too small
                        grow_scratch_1d(0);
                        return get_metadata_item(token, dataset, attempts, band_number, key, domain, value);
                } else { // Return other error code
                        return retval;
                }
        }

        /**
         * Get the widths and heights of all overviews.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band_number The band of interest
         * @param widths      An array of integers to receive the widths of the various
         *                    overviews
         * @param heights     An array of integers to receive the heights of the various
         *                    overviews
         *                    (nominally the smaller of the lengths of the two arrays)
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_overview_widths_heights(long token, int dataset, int attempts, /* */
                        int band_number, int[] widths, int heights[]);

        /**
         * Get the CRS in PROJ.4 format.
         *
         * @param token    A token associated with some uri, options pair
         * @param dataset  0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                 GDALWarp::WARPED) for the warped dataset
         * @param attempts The number of attempts to make before giving up
         * @param crs      The character array in-which to return the PROJ.4 string
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_crs_proj4(long token, int dataset, int attempts, /* */
                        byte[] crs);

        /**
         * Get the CRS in WKT format.
         *
         * @param token    A token associated with some uri, options pair
         * @param dataset  0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                 GDALWarp::WARPED) for the warped dataset
         * @param attempts The number of attempts to make before giving up
         * @param crs      The character array in-which to return the PROJ.4 string
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_crs_wkt(long token, int dataset, int attempts, /* */
                        byte[] crs);

        /**
         * Get the NODATA value for a band.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band        The band number of interest
         * @param nodata      The return-location of the NODATA value
         * @param success     The return-location of the success flag (answer whether or
         *                    not there is a NODATA value)
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_band_nodata(long token, int dataset, int attempts, /* */
                        int band, double[] nodata, int[] success);

        /**
         * Get the minimum and maximum values found in the band.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band        The band number of interest
         * @param approx_okay Is it okay to approximate the value if it is not stored in
         *                    the metadata, or must it be calculated exactly? An integer
         *                    treated as a boolean
         * @param minmax      The return-location of the minimum and maximum
         * @param success     The return-location of the success flag (answer whether or
         *                    not there is a NODATA value)
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_band_min_max(long token, int dataset, int attempts, /* */
                        int band, boolean approx_okay, double[] minmax, int[] success);

        /**
         * Get the data type of a given band.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param band        The band number of interest
         * @param data_type   The return-location of the band_number type (of integral
         *                    type GDALDataType)
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_band_data_type(long token, int dataset, int attempts, /* */
                        int band, int[] data_type);

        /**
         * Get the number of bands.
         *
         * @param token      A token associated with some uri, options pair
         * @param dataset    0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                   GDALWarp::WARPED) for the warped dataset
         * @param attempts   The number of attempts to make before giving up
         * @param band_count The return-location of the band count
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_band_count(long token, int dataset, int attempts, /* */
                        int[] band_count);

        /**
         * Get the width and height.
         *
         * @param token    A token associated with some uri, options pair
         * @param dataset  0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                 GDALWarp::WARPED) for the warped dataset
         * @param attempts The number of attempts to make before giving up
         * @param width_height  The return-location of the width and height
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_width_height(long token, int dataset, int attempts, /* */
                        int[] width_height);

        /**
         * Get pixel data.
         *
         * @param token       A token associated with some uri, options pair
         * @param dataset     0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                    GDALWarp::WARPED) for the warped dataset
         * @param attempts    The number of attempts to make before giving up
         * @param src_window  Please see
         *                    https://gdal.org/api/raster_c_api.html?highlight=rasterio#_CPPv419GDALDatasetRasterIO12GDALDatasetH10GDALRWFlagiiiiPvii12GDALDataTypeiPiiii
         * @param dst_window  Please see
         *                    https://gdal.org/api/raster_c_api.html?highlight=rasterio#_CPPv419GDALDatasetRasterIO12GDALDatasetH10GDALRWFlagiiiiPvii12GDALDataTypeiPiiii
         * @param band_number The band_number number of interest
         * @param type        The desired type of returned pixels (the argument is of
         *                    integral type GDALDataType)
         * @param data        The return-location of the read read data
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_data( /* */
                        long token, /* */
                        int dataset, /* */
                        int attempts, /* */
                        int[] src_window, /* */
                        int[] dst_window, /* */
                        int band_number, /* */
                        int type, /* */
                        byte[] data);

        /**
         * Get the the transform.
         *
         * @param token     A token associated with some uri, options pair
         * @param dataset   0 (or GDALWarp::SOURCE) for the source dataset, 1 (or
         *                  GDALWarp::WARPED) for the warped dataset
         * @param attempts  The number of attempts to make before giving up
         * @param transform The return location for the six double-precision floating
         *                  point number that will be returned
         * @return The number of attempts made (upon success) or a negative error code
         *         (upon failure)
         */
        public static native int get_transform(long token, int dataset, int attempts, /* */
                        double[] transform);
}
