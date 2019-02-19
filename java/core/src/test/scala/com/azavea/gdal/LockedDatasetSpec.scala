package com.azavea.gdal

import org.scalatest._

class LockedDatasetSpec extends FunSpec with Matchers with BeforeAndAfterAll {
  it("Test that dylibs are laoded") {
    LockedDataset
    println("Libs were loaded as expected")
  }
}
