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

class GDALWarp {
    private static final String GDALWARP_BINDINGS = "gdalwarp_bindings";

    public static int GDT_Unknown = 0;
    public static int GDT_Byte = 1;
    public static int GDT_UInt16 = 2;
    public static int GDT_Int16 = 3;
    public static int GDT_UInt32 = 4;
    public static int GDT_Int32 = 5;
    public static int GDT_Float32 = 6;
    public static int GDT_Float64 = 7;
    public static int GDT_CInt16 = 8;
    public static int GDT_CInt32 = 9;
    public static int GDT_CFloat32 = 10;
    public static int GDT_CFloat64 = 11;
    public static int GDT_TypeCount = 12;

    private static native void _init(int size);

    public static void init(int size) {
        System.loadLibrary(GDALWARP_BINDINGS);
        _init(size);
    }

    public static native void deinit();

    public static native long get_token(String uri, String[] options);

    public static native void surrender_token(long token);

    public static native boolean read_data( /* */
            long token, /* */
            int[] src_window, /* */
            int[] dst_window, /* */
            int band_number, /* */
            int type, /* */
            byte[] data);
}
