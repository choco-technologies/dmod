# dmod_dmcomp

The `dmod_dmcomp` library of DMOD: the DEFLATE coder and decoder of
[choco-technologies/dmcomp](https://github.com/choco-technologies/dmcomp), which is the git submodule
`lib/third-party/dmcomp`. There are no sources here, only the build (`CMakeLists.txt`, and the `Makefile` that it writes).

It is the `deflate` method of `Dmod_Compression_*` (`src/system/if/dmod_if_comp.c`). To change the library, change dmcomp
(its tests, sanitizers and size checks are there) and move the submodule to the new commit.
