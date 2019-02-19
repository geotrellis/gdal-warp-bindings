name := "gdal-jni"

lazy val commonSettings = Seq(
  version := "0.0.1" + Environment.versionSuffix,
  scalaVersion := "2.11.12",
  crossScalaVersions := Seq("2.12.8", "2.11.12"),
  organization := "com.azavea",
  description := "GDAL JNI bindings",
  licenses := Seq("Apache-2.0" -> url("http://www.apache.org/licenses/LICENSE-2.0.html")),
  headerLicense := Some(HeaderLicense.ALv2("2019", "Azavea")),
  homepage := Some(url("http://geotrellis.io")),
  publishMavenStyle := true,
  pomIncludeRepository := { _ => false },
  scalacOptions ++= Seq(
    "-deprecation",
    "-unchecked",
    "-language:implicitConversions",
    "-language:reflectiveCalls",
    "-language:higherKinds",
    "-language:postfixOps",
    "-language:existentials",
    "-feature"
  ),
  test in assembly := {},
  shellPrompt := { s => Project.extract(s).currentProject.id + " > " },
  commands ++= Seq(
    Commands.processJavastyleCommand("publish"),
    Commands.processJavastyleCommand("publishSigned")
  ),
  publishArtifact in Test := false,
  publishTo := {
    val nexus = "https://oss.sonatype.org/"
    if (isSnapshot.value)
      Some("snapshots" at nexus + "content/repositories/snapshots")
    else
      Some("releases"  at nexus + "service/local/staging/deploy/maven2")
  },
  pomExtra := (
    <scm>
      <url>git@github.com:jamesmcclain/gdal-warp-bindings.git</url>
      <connection>scm:git:git@github.com:jamesmcclain/gdal-warp-bindings.git</connection>
    </scm>
      <developers>
        <developer>
          <id>jamesmcclain</id>
          <name>James McClain</name>
          <url>http://github.com/jamesmcclain/</url>
        </developer>
      </developers>
    ),
  PgpKeys.useGpg in Global := true,
  PgpKeys.gpgCommand in Global := "gpg"
)

lazy val root = (project in file("."))
  .settings(commonSettings: _*)
  .aggregate(core, native)

lazy val core = project
  .settings(commonSettings: _*)
  .settings(name := "gdal")
  .settings(target in javah := (sourceDirectory in nativeCompile in native).value / "include")
  .settings(libraryDependencies += Dependencies.scalaTest % Test)
  .dependsOn(Environment.dependOnNative(native % Runtime): _*)

lazy val native = project
  .settings(crossPaths := false)
  .settings(name := "gdal-native")
  .settings(sourceDirectory in nativeCompile := sourceDirectory.value)
  .settings(artifactName := { (sv: ScalaVersion, module: ModuleID, artifact: Artifact) =>
    artifact.name + "-" + nativePlatform.value + "-" + module.revision + "." + artifact.extension
  })
  .enablePlugins(JniNative, JniPackage)