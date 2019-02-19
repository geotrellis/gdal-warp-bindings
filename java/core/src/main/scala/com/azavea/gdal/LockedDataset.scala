package com.azavea.gdal

import ch.jodersky.jni.nativeLoader

import java.nio.ByteBuffer

class LockedDataset(val uri: String, val opts: Array[String]) extends Native {

  /**
    * Probably there should be some other metadata methods
    *
    * cellType; dataType; Extent (as an array or in any other type)
    */
  @native def getTransform: Array[Double]
  // this can have a better interface though, more functional
  // (or at least java immutable; means returns smth, but throws an exception in case of failure)
  @native def getPixels(srcWindow: Array[Int], dstWindow: Array[Int], bandNumber: Int, dataType: GDALDataType, data: ByteBuffer): Boolean
  @native def close(): Unit
}

@nativeLoader("gdaljni.0.0")
object LockedDataset {

}
