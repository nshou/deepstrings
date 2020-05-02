# Deepstrings

<!-- Short description -->

## Requirement

Hardware:
- CPU: [Intel Pin-supported CPU](https://software.intel.com/en-us/articles/pin-a-binary-instrumentation-tool-faq)
  - Tested on Xeon E5520

Software:
- Linux OS
  - Tested on RHEL 7.8
- [Intel Pin](https://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool)
  - Tested on pin-3.13-98189-g60a6ef199-gcc-linux

## Build

1. Download Pin

   Download tar ball from [Pin Download Page](https://software.intel.com/en-us/articles/pin-a-binary-instrumentation-tool-downloads). Unzip it and set an environment variable `PIN_ROOT`.

   ```bash
   $ tar zxf pin-3.13-98189-g60a6ef199-gcc-linux.tar.gz
   $ export PIN_ROOT=$(pwd)/pin-3.13-98189-g60a6ef199-gcc-linux
   ```

2. Build Deepstrings

   Get source and run `make` under the source directory.

   ```bash
   $ git clone https://github.com/nshou/deepstrings.git
   $ cd deepstrings
   $ make obj-intel64/deepstrings.so
   ```

   For 32-bit applications,

   ```bash
   $ make obj-ia32/deepstrings.so TARGET=ia32
   ```

## Run

```bash
gcc tests/001_catints.c -o tests/001_catints.out
${PIN_ROOT}/pin -t obj-intel64/deepstrings.so -- tests/001_catints.out
```

This will generate `deepstrings.out` in the same directory.

## License

MIT. See COPYING and NOTICE.
