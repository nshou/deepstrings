#!/bin/sh
#
# Usage: ./test.sh
#        ./test.sh clean
#


#
# t0XX: Preliminary test
#
t000_prerequisites(){
    test -n "${PIN_ROOT}"
    test -f "${PIN_ROOT}"/pin
    test -s "${PIN_ROOT}"/pin
    test -f ../obj-intel64/deepstrings.so
    test -s ../obj-intel64/deepstrings.so
}

t001_inspect(){
    uname -a
    cat /etc/os-release
    cat /proc/cpuinfo
    cat /proc/meminfo
    echo "${PIN_ROOT}"
    "${PIN_ROOT}"/pin -version
}


#
# t1XX: Peripheral function test
#
t100_arg_o(){
    rm -f deepstrings.out
    "${PIN_ROOT}"/pin -t ../obj-intel64/deepstrings.so -- true
    test -f deepstrings.out
    test -s deepstrings.out
    rm -f deepstrings.out
    rm -f t100_arg_o.OUT
    "${PIN_ROOT}"/pin -t ../obj-intel64/deepstrings.so -o t100_arg_o.OUT -- true
    test -f t100_arg_o.OUT
    test -s t100_arg_o.OUT
    test ! -f deepstrings.out
    rm -f t100_arg_o.OUT
    "${PIN_ROOT}"/pin -t ../obj-intel64/deepstrings.so -o -- true 2>&1 | grep -q "NFE: missing argument for -o"
    test ! -f deepstrings.out
}


#
# t2XX: Core function test
#
__t2XX_compile_and_check(){
    o=$(echo "$1" | sed 's/\.c$//')
    t="${o}.OUT"
    d="${o}.DS.OUT"
    gcc -o "${t}" "$@"
    ./"${t}" | grep -q "Hello, World!"
    test "$(strings "${t}" | grep -c "Hello, World!")" -eq 0
    "${PIN_ROOT}"/pin -t ../obj-intel64/deepstrings.so -o "${d}" -- ./"${t}" >/dev/null
    test "$(grep -c "Hello, World!" "${d}")" -gt 0
    rm -f "${t}" "${d}"
}

t200_detect_catints(){
    __t2XX_compile_and_check catints.c
}

t201_detect_xorstr(){
    __t2XX_compile_and_check xorstr.c
}

t202_detect_stackstr(){
    __t2XX_compile_and_check stackstr.c
}

t203_detect_floatstr(){
    __t2XX_compile_and_check floatstr.c
}

t204_detect_encstr(){
    __t2XX_compile_and_check encstr.c -lcrypto
}


#
# Common routines
#
TEST_DATE=$(date -u +%m-%d-%Y_%H-%M-%S)

do_tests(){
    unset TEST_PIDS
    for t in "$@"; do
        l=$(echo "$t" | cut -d " " -f 1)
        (
            (set -o errexit; set -o xtrace; $t 1>"${l}.${TEST_DATE}.LOG" 2>&1)
            # shellcheck disable=SC2181
            # errexit doesn't work as expected inside 'if' test
            if [ $? -eq 0 ]; then
                echo "$t" succeeded.
                true;
            else
                echo "$t" failed.
                false;
            fi
        ) & TEST_PIDS=${TEST_PIDS:+${TEST_PIDS} }$!
    done

    unset RET
    for p in ${TEST_PIDS}; do
        wait "$p"
        RET=$((RET | $?))
    done

    return $((!!RET))
}

clean(){
    rm -f deepstrings.out pintool.log
    rm -f -- *.LOG
    rm -f -- *.OUT
}


#
# Main
#
if [ "$1" = "clean" ]; then
    clean
    exit 0
fi
make obj-intel64/deepstrings.so -C .. || exit 1
do_tests "t000_prerequisites" "t001_inspect" || exit 1
do_tests "t100_arg_o" || exit 1
do_tests "t200_detect_catints" "t201_detect_xorstr" "t202_detect_stackstr" "t203_detect_floatstr" "t204_detect_encstr" || exit 1
echo All tests passed.
