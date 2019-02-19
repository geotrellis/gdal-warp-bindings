package com.azavea.gdal

import org.scalatest._

class GDALWarpSpec extends FunSpec with Matchers with BeforeAndAfterAll {
  it("Test that dylibs are laoded") {
    GDALWarp
    println("Libs were loaded as expected")
  }
}
