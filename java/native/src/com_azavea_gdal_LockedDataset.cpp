#include <stdio.h>
#include <iostream>
#include <string>

#include "Accessors.hpp"

JNIEXPORT void JNICALL Java_io_pdal_Pipeline_initialise
  (JNIEnv *env, jobject obj)
{
    jclass c = env->GetObjectClass(obj);
    // jfieldID uri = env->GetFieldID(c, "uri", "Ljava/lang/String;");
    // jstring jstr = reinterpret_cast<jstring>(env->GetObjectField(obj, uri));
    // it saves a pointer to a dataset into a Java object
    // setHandle(env, obj, new locked_dataset(std::string(env->GetStringUTFChars(jstr, 0))));
}

JNIEXPORT jdoubleArray JNICALL Java_com_azavea_gdal_LockedDataset_getTransform
  (JNIEnv *, jobject)
{

}

JNIEXPORT jboolean JNICALL Java_com_azavea_gdal_LockedDataset_getPixels
  (JNIEnv *, jobject, jintArray, jintArray, jint, jobject, jobject)
{

}

JNIEXPORT void JNICALL Java_com_azavea_gdal_LockedDataset_close
  (JNIEnv *, jobject)
{
    // example of a close method
    // get a C object
    // locked_dataset *ds = getHandle<locked_dataset>(env, obj);
    // ds.close() // close dataset
    // setHandle<int>(env, obj, 0); // forget pointer
    // delete locked_dataset; // remove object
}

