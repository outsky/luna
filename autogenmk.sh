#/bin/sh

set -e
set -u

ALL_O=""
CC="cc"

OBJS="# autogen with $CC -MM"
for t in *.c; do
    ALL_O=$ALL_O${t%%.*}.o" "
    OBJS=$OBJS$(echo -e "\n"$($CC -MM $t))
done

echo "# Autogened at `date +\"%Y/%m/%d %H:%M:%S\"`

BIN = luna

CFLAGS = -g -Wall -std=c99 -D_GNU_SOURCE

LIBS = #empty

ALL_O = $ALL_O

\$(BIN): \$(ALL_O)
	$CC -o \$@ \$(CFLAGS) \$(ALL_O) \$(LIBS)

clean:
	rm -f \$(BIN) \$(ALL_O)

$OBJS" > makefile
