# Deepstrings

Short description

## Requirement

Hardware:
- CPU: [Intel Pin-supported CPU](https://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool)
  - Tested on Xeon E5520

Software:
- Linux OS
  - Tested on RHEL 7.8
- [Intel Pin](https://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool)
  - Tested on pin-3.13-98189-g60a6ef199-gcc-linux

## Build

1. Download Pin

   Download tar ball from [Pin Download Page](https://software.intel.com/en-us/articles/pin-a-binary-instrumentation-tool-downloads).

2. Incorporate Deepstrings

   Deploy files in appropriate directory to prepare for build.

   ```bash
   $ tar zxf pin-<VERSION>.tar.gz
   $ mkdir pin-<VERSION>/source/tools/deepstrings
   $ cd pin-<VERSION>/source/tools/deepstrings
   $ cp ~/deepstrings/deepstrings.cpp .
   $ cp ../MyPinTool/makefile* .
   $ sed -i 's/MyPinTool/deepstrings/' makefile.rules
   ```

3. Build Deepstrings

   Run `make` under `pin-<VERSION>/source/tools/deepstrings` directory. For 64-bit application,

   ```bash
   $ make obj-intel64/deepstrings.so
   ```

   For 32-bit application,

   ```bash
   $ make obj-ia32/deepstrings.so TARGET=ia32
   ```

## Run

Under `pin-<VERSION>/source/tools/deepstrings` directory,

```bash
../../../pin -t obj-intel64/deepstrings.so -- <YOUR_APP>
```

then `deepstrings.out` and `deepstrings.out.debug` are produced in the same directory.

## License

MIT.





- The purpose and goal of the project (e.g. This tool is designed to reverse the string encryption performed by malware X...)

- A description of how the tool works to solve that problem

- A list of files in the project and a short description of what each files are

- Installation and usage instructions, as clear as possible (the ideal would be a setup script or a list of copy-paste commands that are environment independent)

- Any limitations faced by the tool, and how it could be improved in the future
