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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if defined(__APPLE__)
#include <arpa/inet.h>
#define htobe16(x) htons(x)
#define htobe32(x) htonl(x)
#define htobe64(x) htonll(x)
#elif defined(__MINGW32__)
#include <stdint.h>
#include <winsock2.h>
#define htobe16(x) htons(x)
#define htobe32(x) htonl(x)

uint64_t htobe64(uint64_t x)
{
    uint64_t retval;
    uint32_t *p1 = (uint32_t *)&retval;
    uint32_t *p2 = (uint32_t *)&x;
    p1[0] = htonl(p2[1]);
    p1[1] = htonl(p2[0]);
    return retval;
}
#else
#include <endian.h>
#endif

#include "com_azavea_gdal_GDALWarp.h"
#include "bindings.h"

const int MAX_OPTIONS = 1 << 10;
int gc_lock = 0;

JNIEXPORT void JNICALL Java_com_azavea_gdal_GDALWarp__1init(JNIEnv *env, jobject obj, jint size)
{
    gc_lock = (getenv("GDALWARP_GC_LOCK") != NULL); // XXX enabling this might be unsafe but might lead to better performance
    init(size);
}

JNIEXPORT void JNICALL Java_com_azavea_gdal_GDALWarp_deinit(JNIEnv *env, jobject obj)
{
    deinit();
}

JNIEXPORT jlong JNICALL Java_com_azavea_gdal_GDALWarp_get_1token(JNIEnv *env, jobject obj, jstring _uri, jobjectArray _options)
{
    const char *uri = (*env)->GetStringUTFChars(env, _uri, NULL);
    char const *options[MAX_OPTIONS];
    jsize len = (*env)->GetArrayLength(env, _options);

    options[len] = NULL;
    for (jint i = 0; (i < len) && (i < MAX_OPTIONS); ++i)
    {
        jstring option = (*env)->GetObjectArrayElement(env, _options, i);
        options[i] = (*env)->GetStringUTFChars(env, option, NULL); //GetStringUTFRegion?
    }

    jlong token = get_token(uri, options);

    for (jint i = 0; (i < len) && (i < MAX_OPTIONS); ++i)
    {
        jstring option = (*env)->GetObjectArrayElement(env, _options, i);
        (*env)->ReleaseStringUTFChars(env, option, options[i]);
    }
    (*env)->ReleaseStringUTFChars(env, _uri, uri);

    return token;
}

JNIEXPORT jint JNICALL Java_com_azavea_gdal_GDALWarp_get_1overview_1widths_1heights(JNIEnv *env, jclass obj,
                                                                                    jlong token,
                                                                                    jint dataset,
                                                                                    jint attempts,
                                                                                    jintArray _widths,
                                                                                    jintArray _heights)
{
    jint *widths = (*env)->GetIntArrayElements(env, _widths, NULL);
    jint *heights = (*env)->GetIntArrayElements(env, _heights, NULL);
    int width_length = (*env)->GetArrayLength(env, _widths);
    int height_length = (*env)->GetArrayLength(env, _heights);
    int max_length = width_length < height_length ? width_length : height_length;
    jint retval = get_overview_widths_heights(token, dataset, attempts, (int *)widths, (int *)heights, max_length);
    (*env)->ReleaseIntArrayElements(env, _heights, heights, 0);
    (*env)->ReleaseIntArrayElements(env, _widths, widths, 0);

    return (retval == -ENOENT ? -1 : (retval == -EAGAIN ? 0 : retval));
}

JNIEXPORT jint JNICALL Java_com_azavea_gdal_GDALWarp_get_1crs_1proj4(JNIEnv *env, jclass obj,
                                                                     jlong token,
                                                                     jint dataset,
                                                                     jint attempts,
                                                                     jbyteArray _crs)
{
    jbyte *crs = (*env)->GetByteArrayElements(env, _crs, NULL);
    int max_size = (*env)->GetArrayLength(env, _crs);
    jint retval = get_crs_proj4(token, dataset, attempts, (char *)crs, max_size);
    (*env)->ReleaseByteArrayElements(env, _crs, crs, 0);

    return (retval == -ENOENT ? -1 : (retval == -EAGAIN ? 0 : retval));
}

JNIEXPORT jint JNICALL Java_com_azavea_gdal_GDALWarp_get_1crs_1wkt(JNIEnv *env, jclass obj,
                                                                   jlong token,
                                                                   jint dataset,
                                                                   jint attempts,
                                                                   jbyteArray _crs)
{
    jbyte *crs = (*env)->GetByteArrayElements(env, _crs, NULL);
    int max_size = (*env)->GetArrayLength(env, _crs);
    jint retval = get_crs_wkt(token, dataset, attempts, (char *)crs, max_size);
    (*env)->ReleaseByteArrayElements(env, _crs, crs, 0);

    return (retval == -ENOENT ? -1 : (retval == -EAGAIN ? 0 : retval));
}

JNIEXPORT jint JNICALL Java_com_azavea_gdal_GDALWarp_get_1band_1nodata(JNIEnv *env, jclass obj,
                                                                       jlong token,
                                                                       jint dataset,
                                                                       jint attempts,
                                                                       jint band,
                                                                       jdoubleArray _nodata,
                                                                       jintArray _success)
{
    double *nodata = (*env)->GetDoubleArrayElements(env, _nodata, NULL);
    jint *success = (*env)->GetIntArrayElements(env, _success, NULL);
    jint retval = get_band_nodata(token, dataset, attempts, band, nodata, (int *)success);
    (*env)->ReleaseIntArrayElements(env, _success, success, 0);
    (*env)->ReleaseDoubleArrayElements(env, _nodata, nodata, 0);

    return (retval == -ENOENT ? -1 : (retval == -EAGAIN ? 0 : retval));
}

JNIEXPORT jint JNICALL Java_com_azavea_gdal_GDALWarp_get_1band_1data_1type(JNIEnv *env, jclass obj,
                                                                           jlong token,
                                                                           jint dataset,
                                                                           jint attempts,
                                                                           jint band,
                                                                           jintArray _data_type)
{
    jint *data_type = (*env)->GetIntArrayElements(env, _data_type, NULL);
    jint retval = get_band_data_type(token, dataset, attempts, band, (int *)data_type);
    (*env)->ReleaseIntArrayElements(env, _data_type, data_type, 0);

    return (retval == -ENOENT ? -1 : (retval == -EAGAIN ? 0 : retval));
}

JNIEXPORT jint JNICALL Java_com_azavea_gdal_GDALWarp_get_1band_1count(JNIEnv *env, jclass obj,
                                                                      jlong token,
                                                                      jint dataset,
                                                                      jint attempts,
                                                                      jintArray _band_count)
{
    jint *band_count = (*env)->GetIntArrayElements(env, _band_count, NULL);
    jint retval = get_band_count(token, dataset, attempts, (int *)band_count);
    (*env)->ReleaseIntArrayElements(env, _band_count, band_count, 0);

    return (retval == -ENOENT ? -1 : (retval == -EAGAIN ? 0 : retval));
}

jint JNICALL Java_com_azavea_gdal_GDALWarp_get_1width_1height(JNIEnv *env, jclass obj,
                                                              jlong token,
                                                              jint dataset,
                                                              jint attempts,
                                                              jintArray _width_height)
{
    jint *width_height = (*env)->GetIntArrayElements(env, _width_height, NULL);

    jint retval = get_width_height(token, dataset, attempts, (int *)width_height, (int *)(width_height + 1));
    (*env)->ReleaseIntArrayElements(env, _width_height, width_height, 0);

    return (retval == -ENOENT ? -1 : (retval == -EAGAIN ? 0 : retval));
}

JNIEXPORT jint JNICALL Java_com_azavea_gdal_GDALWarp_get_1data(JNIEnv *env, jobject obj,
                                                               jlong token,
                                                               jint dataset,
                                                               jint attempts,
                                                               jintArray _src_window,
                                                               jintArray _dst_window,
                                                               jint band_number,
                                                               jint type,
                                                               jbyteArray _data)
{
    jint *src_window = (*env)->GetIntArrayElements(env, _src_window, NULL);
    jint *dst_window = (*env)->GetIntArrayElements(env, _dst_window, NULL);
    int length = (*env)->GetArrayLength(env, _data);
    void *data = NULL;

    if (gc_lock)
    {
        data = (*env)->GetPrimitiveArrayCritical(env, _data, NULL);
    }
    else
    {
        data = (*env)->GetByteArrayElements(env, _data, NULL);
    }
    jint retval = get_data(token, dataset, attempts, (int *)src_window, (int *)dst_window, band_number, type, data);
    switch (type)
    {
    case com_azavea_gdal_GDALWarp_GDT_Int16:
    case com_azavea_gdal_GDALWarp_GDT_UInt16:
    case com_azavea_gdal_GDALWarp_GDT_CInt16:
        for (int i = 0; i <= length - sizeof(uint16_t); i += sizeof(uint16_t))
        {
            uint16_t *ptr = (uint16_t *)(data + i);
            *ptr = htobe16(*ptr);
        }
        break;
    case com_azavea_gdal_GDALWarp_GDT_Int32:
    case com_azavea_gdal_GDALWarp_GDT_UInt32:
    case com_azavea_gdal_GDALWarp_GDT_CInt32:
    case com_azavea_gdal_GDALWarp_GDT_Float32:
    case com_azavea_gdal_GDALWarp_GDT_CFloat32:
        for (int i = 0; i <= length - sizeof(uint32_t); i += sizeof(uint32_t))
        {
            uint32_t *ptr = (uint32_t *)(data + i);
            *ptr = htobe32(*ptr);
        }
        break;
    case com_azavea_gdal_GDALWarp_GDT_Float64:
    case com_azavea_gdal_GDALWarp_GDT_CFloat64:
        for (int i = 0; i <= length - sizeof(uint64_t); i += sizeof(uint64_t))
        {
            uint64_t *ptr = (uint64_t *)(data + i);
            *ptr = htobe64(*ptr);
        }
    }
    if (gc_lock)
    {
        (*env)->ReleasePrimitiveArrayCritical(env, _data, data, 0);
    }
    else
    {
        (*env)->ReleaseByteArrayElements(env, _data, data, 0);
    }
    (*env)->ReleaseIntArrayElements(env, _dst_window, dst_window, JNI_ABORT);
    (*env)->ReleaseIntArrayElements(env, _src_window, src_window, JNI_ABORT);

    return (retval == -ENOENT ? -1 : (retval == -EAGAIN ? 0 : retval));
}

JNIEXPORT jint JNICALL Java_com_azavea_gdal_GDALWarp_get_1transform(JNIEnv *env, jclass obj,
                                                                    jlong token,
                                                                    jint dataset,
                                                                    jint attempts,
                                                                    jdoubleArray _transform)
{
    double *transform = (*env)->GetDoubleArrayElements(env, _transform, NULL);
    jint retval = get_transform(token, dataset, attempts, transform);
    (*env)->ReleaseDoubleArrayElements(env, _transform, transform, 0);

    return (retval == -ENOENT ? -1 : (retval == -EAGAIN ? 0 : retval));
}
