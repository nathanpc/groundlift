#!/bin/sh

### wchar-size.sh
### Prints out the size in bytes of the current wchar_t implementation used by
### the default compiler.
###
### Author: Nathan Campos <nathan@innoveworkshop.com>

testexec=/tmp/testwchar

gcc -Wall -o "$testexec" -xc - << EOF
#include <stdio.h>
#include <wchar.h>

int main() {
    printf("%lu", sizeof(wchar_t));
    return 0;
}
EOF

$testexec
