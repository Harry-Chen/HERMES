# HERMES

HERMES, a.k.a. s**H**allow dir**E**ctory st**R**ucture **M**any-fil**E** file**S**ystem, is a filesystem base on FUSE designed for numerous small files with simple directory structure, on which occasion common file systems has a serious performance degradation.

## Authors & License

The authors of `HERMES` are listed below:

* [Shengqi Chen](https://github.com/Harry-Chen)
* [Jiajie Chen](https://github.com/jiegec)
* [Xiaoyi Liu](https://github.com/CircuitCoder)
* [Jian Gao](https://github.com/IcicleF)

This project is an assignment of `Basics of Stroage Technology` course, Spring 2019, Tsinghua University. All code created by the authors is licensed under the MIT license, while open source code used in this project is subjected to their own licenses.

## Dependencies

### Project

* `clang++` >= 7.0
* `gcc` >= 5.0
* `CMake` >= 2.8

### Backends

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
LevelDB and RocksDB need directories, while BerkeleyDB and Vedis need files.

You can use `--help` to see the complete help text and `--version` to see the compilation time and backend version of HERMES.

## Tests

There are several tests in `tests` directory:

* `read_write_perf.sh`: Test the read & write performance of different backends.
* `file_list_perf.sh`: Test the performance of file creation & enumeration of different backends.
* `test_correctness.sh`: Test the implementation correctness of different backends.

The first two tests requires `fio` to run, and will generate reports in `results` directory.

## Implementation

Refer to `doc/report.pdf` for more details (in Chinese).
