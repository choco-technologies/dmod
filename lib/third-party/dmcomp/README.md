# dmcomp (third-party)

DEFLATE (RFC 1951) coder and decoder of [choco-technologies/dmcomp](https://github.com/choco-technologies/dmcomp),
commit `7314bf2` (`feat/inflate-workspace`). MIT licensed, see `LICENSE.MIT`.

It is the `deflate` method of `Dmod_Compression_*` (`src/system/if/dmod_if_comp.c`): the DMFC files of a module that
`todmfc` makes with `deflate` are ~20 % smaller than the `fastlz` ones. The decoder is 1.7 kB of code on Cortex-M, the
encoder is for the host (`DMOD_USE_DMCOMP_ENCODER`).

**Do not edit the files here** – change them in dmcomp and copy them again (the tests, the sanitizers and the
size checks of the CI are there).
