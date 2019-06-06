# HERMES

HERMES, a.k.a. `sHallow dirEctory stRucture Many-filE fileSystem`, is a filesystem base on FUSE designed for numerous small files with simple directory structure, on which occasion common file systems has a serious performance degradation.

This project is an assignment of `Basics of Stroage Technology` course, Spring 2019, Tsinghua University. All code created by the authors is licensed under the MIT license, while open source code used in this project is subjected to their own licenses.

## Dependencies

### Project

* `clang++` >= 7.0
* `gcc` >= 5.0
* `CMake` >= 2.8

#### Backends

HERMES supports several key-value databases as its backend, and you can choose one when configuring cmake project. 
LevelDB will be used as the default backend if you do not specify one. According to your choice, HERMES would depends on one of:

* `LevelDB` >= 1.20
* `RocksDB` >= 5.8.8
* `BerkeleyDB` >= 5.3.28
* `Vedis`: no dependencies (using `git-submodule` to download source automatically)

You can either install prebuilt development packages (such as `libleveldb-dev`) or compile and install them manually.
See `CMakeModules/*.cmake` for how to specify non-default library locations.

## Compilation & Running

```bash
mkdir build && cd build
CXX=clang++ cmake .. -DBACKEND=[LevelDB/RocksDB/BerkeleyDB/Vedis]
cmake --build .
```

The executable can be located at `build/src/HERMES`. 

Besides common `fuse` options, filesystem-specific options are:

```text
--metadev=/path/to/meta # location to save the metadata
--filedev=/path/to/file # location to save the file content
```

Note that different backends requires different types `metadev` and `filedev`.
LevelDB and RocksDB needs directories, while Vedis needs files.

You can use `--help` to see the complete help text and `--version` to see the compilation time and backend version of HERMES.

## Implementation

### Storing Policy

`HERMES` use a key-value store as its backend.
All items in the file system are mapped to two key-value pairs, respectively its metadata and content.

#### Files

Note that some types of file, including hard link, device, or pipe are not supported.

#### Directories

To be done

### Internal Data Structure

To be done
