package com.azavea.gdal

trait Native {
  protected var nativeHandle = 0l // C++ pointer
  def ptr(): Long = nativeHandle
  def close(): Unit
}