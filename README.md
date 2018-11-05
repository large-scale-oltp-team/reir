# RErational Intermediate Representation(REIR)

## Setup and Build

We usually use Ubuntu 16 or 18, so following steps shows packaging command for these environment.

### Build Prerequiste

#### [foedus](https://github.com/large-scale-oltp-team/foedus_code/) needs to be built and installed.
It's not necessary to install foedus in the standard directory, e.g. /usr/local/lib64. You can install to any directory in order to keep the environment clean.
To do this, before you build and install foedus, when you issue cmake command to setup the build directory, specify `CMAKE_INSTALL_PREFIX` option with the directory you like to install:

```
$ cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/local ..
```
then `make install` will install foedus using this directory as root, e.g. /local/lib64/libfoedus-core.so

#### [leveldb](https://github.com/google/leveldb)

You can install to standard location
```
$ apt install libleveldb-dev
```
or build and install to customized location.
To do this, before you build and install leveldb, specify the directory for `CMAKE_INSTALL_PREFIX` option on the cmake command line.

#### [LLVM](https://llvm.org/) 6

REIR uses LLVM for two purposes.
- as the toolchain to compile and build reir libs.
- to call llvm api to implement code generator in reir.

For the former, the version requirement is not so strict and either version 6,7 (or even 8?) can be used.
For the latter, you need LLVM version 6.0.0.
You MUST build LLVM-6.0.6 with _DEBUG SYMBOL_ with `cmake -DCMAKE_BUILD_TYPE=Debug `


Or you can [build](https://llvm.org/docs/CMake.html) it from the source code. To do this, before you build and install llvm, specify the directory for `CMAKE_INSTALL_PREFIX` option on the cmake command line .
Also, it's required LLVM is build with RTTI enabled. (See [LLVM manual](https://llvm.org/docs/CMake.html) to do this) 

#### [ninja](https://ninja-build.org/manual.html) for build efficiency

Install from api repo:

```
$ apt install ninja-build
```

### Build Steps

1. clone reir git repository

```
$ git clone https://github.com/large-scale-oltp-team/reir.git
```
2. update submodules with the following command to retrieve third-party code.

```
$ cd reir
$ git submodule update --init --recursive
```
3. create `build` dir. and move there:

```
$ mkdir build
$ cd build
```

4. Issue cmake command to generate build files. There are several options depending on your environment.

* If prereq libs are all installed on standard location, e.g. /usr/local/lib, you need to specify only build type and compiler:
```
$ cmake -G Ninja -DLLVM_PATH=<path to your llvm-6 with debug symbol path> -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=<path to your compiler> ../
```

By default apt installs clan 6 with "-6.0" prefix, so to use clang 6, you need to specify it like this:
```
$ cmake -G Ninja -DLLVM_PATH=<path to your llvm-6 with debug symbol path> -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=/usr/bin/clang++-6.0 ../
```


* If you installed prereq libs to the customized location, specify `CMAKE_PREFIX_PATH` or `CMAKE_INSTALL_PREFIX` to tell cmake command the installed directory location.
`CMAKE_PREFIX_PATH` is used for libs' search path, while `CMAKE_INSTALL_PREFIX` can be used both for search path and install path. If you want to install REIR to customized location together with prereq libs,
`CMAKE_INSTALL_PREFIX` is enough.
```
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=<path to customized install root> -DCMAKE_CXX_COMPILER=<path to your compiler>  ../
```

* If you are using different version LLVM for compile and code generation, you need to specify `LLVM_PATH` to tell the location where LLVM 6 is located.
```
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLLVM_PATH=<path to llvm install dir.> -DCMAKE_CXX_COMPILER=<path to your compiler> ../
```
where `<path to llvm install dir.>` is the root dir. used for llvm installation, i.e. it contains `bin`, `lib` and `include` directories.

5. build reir with ninja command :
```
$ ninja
```

6. Verify build. For example, you can run reirc command to execute the sample rir files:
```
$ src/reir/reirc -f ../examples/print_int.rir
```
Or you can run testcases:
```
$ ninja test
```

7. install REIR
```
$ ninja install
```
