# InBFWeTrust

## Build
```
mkidr build
cd build
cmake -D CMAKE_BUILD_TYPE=Release ..
make
```
## Windows build

```
mkdir build-win
cd build-win
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_TOOLCHAIN_FILE=../mingw_cross_toolchain.cmake ..
make
```
