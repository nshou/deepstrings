# Deepstrings

Short description

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

   To cross-compile for 32-bit version,

   ```bash
   $ make obj-ia32/deepstrings.so TARGET=ia32
   ```

## Run

```bash
${PIN_ROOT}/pin -t obj-intel64/deepstrings.so -- /bin/echo hello
```

This will generate `deepstrings.out` in the same directory.

## License

MIT.





- The purpose and goal of the project (e.g. This tool is designed to reverse the string encryption performed by malware X...)

- A description of how the tool works to solve that problem

- A list of files in the project and a short description of what each files are

- Installation and usage instructions, as clear as possible (the ideal would be a setup script or a list of copy-paste commands that are environment independent)

- Any limitations faced by the tool, and how it could be improved in the future
