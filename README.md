# Zepo
![zepo](https://socialify.git.ci/klrohias/zepo/image?description=1&font=Jost&forks=1&issues=1&language=1&name=1&owner=1&pattern=Solid&pulls=1&stargazers=1&theme=Auto)  
A package manager for C/C++.  

## Quick Start
### Use CMake with Zepo:  
`package.json`:  
```json
{
  "name": "test-prog",
  "dependencies": {
    "some-library": "^0.1.0"
  }
}
```

and run CMake:
```shell
cmake -B ./build -DCMAKE_TOOLCHAIN_FILE=/path/to/zepo-cmake-toolchain.cmake
cmake --build ./build
```

## Build Zepo
Currently, vcpkg is needed when building Zepo,
the vcpkg packages that are necessary for building Zepo are in the `vcpkg.json`,
so just configure CMake with vcpkg and build this project directly.

# License
Currently, **there is no an open-source license so this is not an open-source software in definition**,
we will add an open-source license when we think it is time to.