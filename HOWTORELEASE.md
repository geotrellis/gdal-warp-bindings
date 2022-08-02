# Create a New Release

Follow these steps:

- [ ] Ensure master is up to date
- [ ] Update changelog like in https://github.com/geotrellis/gdal-warp-bindings/commit/9bb786617310bd4e75950d8a0e14ed9a2f2f5952 and commit
- [ ] Tag CHANGELOG commit with `git tag -a vx.y.z -m "vx.y.z" && git push --tags`.  Note that the leading "v" must be part of the tag name, it is not part of the hypothetical tag version.
- [ ] Wait for CI to publish release to OSS SonaType
- [ ] Login to OSS Sonatype https://oss.sonatype.org with credentials in password manager
- [ ] Go to https://oss.sonatype.org/#stagingRepositories and filter by package `gdal-warp-bindings`
- [ ] Verify that the JAR package contains the bundled shared libs
- [ ] Select staging repo and press `Close` button on the top of the table with repos
- [ ] Wait for close process to complete
- [ ] Select staging repo again and press Release button. It will be immediately published into sonatype releases repo and synced with maven central ~ in 10 minutes
- [ ] Create a new GitHub release https://github.com/geotrellis/gdal-warp-bindings/releases/new
