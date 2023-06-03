# Sycophant

> **Warning** Sycophant is still in the alpha stages, and
> should not be relied upon in any capacity.
> Not all planned functionality is present yet, and using
> Sycophant in general may cause stability issues in the target
> process.

Sycophant is a `LD_PRELOAD` library that loads a python interpreter into the target process and exposes mechanisms to install hooks into arbitrary functions and inspect the running process, all from within the ease of use of python.`

## Configuring and Building

The following steps describe how to build Sycophant, it should be consistent for Linux, macOS, and Windows, but macOS and Windows remain untested.

**NOTE:** The minimum C++ standard to build Sycophant is C++17.

### Prerequisites

To build Sycophant, ensure you have the following build time dependencies:
 * git
 * meson
 * ninja
 * g++ >= 11 or clang++ >= 11

### Configuring

You can build Sycophant with the default options, all of which can be found in [`meson_options.txt`](meson_options.txt). You can change these by specifying `-D<OPTION_NAME>=<VALUE>` at initial meson invocation time, or with `meson configure` in the build directory post initial configure.

To change the install prefix, which is `/usr/local` by default ensure to pass `--prefix <PREFIX>` when running meson for the first time.

In either case, simply running `meson build` from the root of the repository will be sufficient and place all of the build files in the `build` subdirectory.

### Building

Once you have configured Sycophant appropriately, to simply build and install simply run the following:

```
$ ninja -C build
$ ninja -C build test # Optional: Run Tests
$ ninja -C build install
```

This will build and install Sycophant into the default prefix which is `/usr/local`, to change that see the configuration steps above.

### Notes to Package Maintainers

If you are building Sycophant for inclusion in a distributions package system then ensure to set `DESTDIR` prior to running meson install.

There is also a `bugreport_url` configuration option that is set to this repositories issues tracker by default, it is recommended to change it to your distributions bug tracking page.


## License

Sycophant is licensed under the [BSD 3-Clause](https://spdx.org/licenses/BSD-3-Clause.htm) license. The full text of the license can be found in the [`LICENSE`](./LICENSE) file.
