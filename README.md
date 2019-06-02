# HERMES

HERMES, a.k.a. `sHallow dirEctory stRucture Many-filE fileSystem`, is a filesystem base on FUSE designed for numerous small files with simple directory stucture, on which occasion common file systems has a serious performance degradation.

This project is an assignment of `Basics of Stroage Technology` course, Spring 2019, Tsinghua University. All code created by the authors is licensed under the MIT license, while open source code used in this project is subjected to their own licenses.

## Compilation & Running

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

The executable can be located at `build/src/HERMES`. Supported options are:

```text
To be done
```

## Implementation

### Name Mapping

`HERMES` use a key-value store as its backend. All entries in the file system are mapped to a key-value pair in the store with the following scheme:

* `/path/to/directory` -> `D/path/to/directory`, which contains metadata of the directory as long as a list of its contents (owner, permission, create time, etc.)
* `/path/to/file` -> `/path/to/file`, which contains the content of the file (with no metadata), or destinations of symbolic links

Note that some types of file, including hard link, device, or pipe are not supported.

### Internal Data Structure

To be done
