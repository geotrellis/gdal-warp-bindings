# JNI bindings quick intro

## Generating JNI templates by having Java interfaces annotated with `@native` notation

In a project `core` create any class that extends `Native` and / or have some
`@native` methods. After that do the following:

```bash
> ./sbt
> projet core
> javah
```

`javah` command generates native JNI files in a `native` project.
The output files would be placed into the `native/include` directory

## Generating a CMake sample file for the `native` project

This section is probably not required for you, as usually I write it manually.

```bash
> sbt
> project native
> nativeInit cmake gdaljni # gdaljni here is a project lib name
```

## Build native files

```bash
> ./sbt
> project native
> nativeCompile
```

## Check that everything got caught by the `core` project

```bash
> ./sbt
> project core
> test
```

It will create a singleton, and print "Hello World" what would mean that 
JNI bindings were loaded by Java loader.
