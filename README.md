
# Devise a novel "rowhammer" attack

Because our work is based on Google's existing "Rowhammer" attack, so we first simply cloned Google's code: https://github.com/google/rowhammer-test

We explored three methods to improve the efficiency of existing: Non-temporal (NT) load, eviction pattern and "parallel attack".

How to run the test:

```
./make.sh
./rowhammer_test to run Google's single sided attack.
./double_sided_rowhammer to run Google's double sided attack.
./rowhammer_NTLoad to run our "Rowhammer" attack with NT load.
./rowhammer_MemCraft to run our "Rowhammer" attack with eviction patterns.
./rowhammer_multithread to run our parallel "Rowhammer" attack.
```
Note 1: The rowhammer_NTLoad should only work with Intel's chips that support SSE4.1 because the NT load instruction is part of the SSE4.1 instruction set.

Note 2: rowhammer_multithread relies on the compiler support for OpenMP, so it may not compile directly on Mac as Mac's default g++/gcc is too old to support OpenMP, but one can upgrade the compiler manually.
