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
#include "com_azavea_gdal_GDALWarp.h"
#include "bindings.h"

const int MAX_OPTIONS = 1 << 10;
int gc_lock = 0;

JNIEXPORT void JNICALL Java_com_azavea_gdal_GDALWarp__1init(JNIEnv *env, jobject obj, jint size, jint copies)
{
    gc_lock = (getenv("GDALWARP_GC_LOCK") != NULL); // XXX enabling this might be unsafe
    init(size, copies);
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

JNIEXPORT void JNICALL Java_com_azavea_gdal_GDALWarp_surrender_1token(JNIEnv *env, jobject obj, jlong token)
{
    surrender_token(token);
}

jboolean JNICALL Java_com_azavea_gdal_GDALWarp_get_1width_1height(JNIEnv *env, jclass obj, jlong token, jint attempts, jintArray _width_height)
{
    int *width_height = (*env)->GetIntArrayElements(env, _width_height, NULL);

    jboolean retval = get_width_height(token, attempts, width_height, width_height + 1);
    (*env)->ReleaseIntArrayElements(env, _width_height, width_height, 0);

    return retval;
}

JNIEXPORT jboolean JNICALL Java_com_azavea_gdal_GDALWarp_get_1data(JNIEnv *env, jobject obj,
                                                                   jlong token,
                                                                   jint attempts,
                                                                   jintArray _src_window,
                                                                   jintArray _dst_window,
                                                                   jint band_number,
                                                                   jint type,
                                                                   jbyteArray _data)
{
    int *src_window = (*env)->GetIntArrayElements(env, _src_window, NULL);
    int *dst_window = (*env)->GetIntArrayElements(env, _dst_window, NULL);
    void *data = NULL;

    if (gc_lock)
    {
        data = (*env)->GetPrimitiveArrayCritical(env, _data, NULL);
    }
    else
    {
        data = (*env)->GetByteArrayElements(env, _data, NULL);
    }
    jboolean retval = get_data(token, attempts, src_window, dst_window, band_number, type, data);
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

    return retval;
}
