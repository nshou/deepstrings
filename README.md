# Deepstrings

Deepstrings detects every string embedded in binary files to help work on malware analysis, CTF, and all the other types of reverse engineering.

## Quick Example

`stackstr.c` in tests directory might look weird.

```bash
$ cat tests/stackstr.c
#include <stdio.h>

int main(void){
    long local1 = 0x0a21646c726f;
    int local2 = 0x57202c6f;
    int local3 = 0x6c6c6548;
    long local4 = 0;

    (void)local1;
    (void)local2;
    (void)local3;
    printf("%s", ((char *)&local4) + 8);
    return 0;
}
```

.. but actually, it is a binary printing out "Hello, World!"

```bash
$ gcc tests/stackstr.c
$ ./a.out
Hello, World!
```

This is one of the most common obfuscating techniques called *stack strings*. Attackers often use those techniques in malware to hide malicious strings, a URL of botnet IRC server, for example. You cannot find the hidden strings with simple tools like `strings` (and that is the main purpose of the attackers working on obfuscating, i.e. avoid being detected easily.)

```bash
$ strings a.out | grep Hello
$                              # no result
```

As shown below, however, Deepstrings reveals the string "Hello, World!".

```bash
$ pin -t obj-intel64/deepstrings.so -o /dev/stdout -- ./a.out | grep Hello
insptr:0x7f03bda0f181 hdptr:0x7ffc46d5e220 len:14 str:Hello, World!
```

## Requirement

Hardware:
- CPU: [Intel Pin-supported CPU](https://software.intel.com/en-us/articles/pin-a-binary-instrumentation-tool-faq)
  - Tested on Xeon E5520, Core i5-8250U

Software:
- Linux / OS X / Windows
  - Tested on RHEL 7.8, Debian 10.4 and 10.7, Windows 10
- [Intel Pin](https://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool)
  - Tested on pin 3.13 and 3.17

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
$ ${PIN_ROOT}/pin -t obj-intel64/deepstrings.so -- ./${TARGET_BINARY}
```

The result file `deepstrings.out` will be generated in the same directory.

## Caveats

Deepstrings is a dynamic analysis tool, meaning it executes the target binary as a child process. Make sure you run this tool in an isolated environment if the binary is considered malicious.

Because Deepstrings deals with the target binary as it is, the result is still susceptible to the anti-analysis techniques such as VM detection.

## License

MIT. See COPYING and NOTICE.
