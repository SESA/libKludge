# libKludge

# Introduction
LibKludge allows users of bash to load executables into address space of the main process and execute them like builtin commands.
It is strongly dependent on [Sloader](https://github.com/akawashiro/sloader) for loading and linking.

# Current status
LibKludge is in active development and far from reliable. As of now, it supports loading an executable and executing it an arbitrary amount of times. Pipes are supported.
Loading a second executable leads to the first one being no longer usable. This is a clear TODO.

# Examples
Assuming the shared object enablex that can be loaded using bash's enable is located in the current folder.

```bash
enable -f ./enablex enablex #load and activate builtin enablex
enablex load /usr/bin/jq #load executable
enablex run jq --help #run executable with arguments
```

# Installation
init submodules:
```bash
git submodule init
git submodule update
```
As of now, sloader_dl.cc.o dyn_loader.cc.o libc_mapping.cc.o utils.cc.o need to be using cmake in the sloader fork submodule. (see ./sloader/README)
Then, in the bash submodule, the configure script can be run (see ./bash/INSTALL)
Finally, src/* and the sloader object files need to be copied to ./bash/examples/loadbles where make -f Makefile.enx can be executed.

# TODOs 

1. provide basic functionality
    1. make sure multiple executables can be loaded and executed
    2. restore data sections after each enablex run call
    3. run any on_exit functions that were registered after entering the program at its exit
    4. If not covered by 1.3.: rewrite libc memory allocation functions to keep track of dynamically allocated memory that is to be deallocated after the program exits.
2. provide installation instructions
    1. provide a single build script
    2. So far it's unclear how compatible plugins are with different bash versions
3. increase platform support (so far only x86_64 is supported)


