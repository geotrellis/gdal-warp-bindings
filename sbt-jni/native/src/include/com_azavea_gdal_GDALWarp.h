/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_azavea_gdal_GDALWarp */

#ifndef _Included_com_azavea_gdal_GDALWarp
#define _Included_com_azavea_gdal_GDALWarp
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_azavea_gdal_GDALWarp
 * Method:    getTransform
 * Signature: ()[D
 */
JNIEXPORT jdoubleArray JNICALL Java_com_azavea_gdal_GDALWarp_getTransform
  (JNIEnv *, jobject);

/*
 * Class:     com_azavea_gdal_GDALWarp
 * Method:    getPixels
 * Signature: ([I[IILcom/azavea/gdal/GDALDataType;Ljava/nio/ByteBuffer;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_azavea_gdal_GDALWarp_getPixels
  (JNIEnv *, jobject, jintArray, jintArray, jint, jobject, jobject);

/*
 * Class:     com_azavea_gdal_GDALWarp
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_azavea_gdal_GDALWarp_close
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif