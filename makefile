# Autogened at 2019/08/30 16:03:27

BIN = luna

CFLAGS = -g -Wall -std=c99 -D_GNU_SOURCE

LIBS = #empty

ALL_O = htable.o lasm.o lib.o list.o lvm.o main.o 

$(BIN): $(ALL_O)
	cc -o $@ $(CFLAGS) $(ALL_O) $(LIBS)

clean:
	rm -f $(BIN) $(ALL_O)

# autogen with cc -MM
htable.o: htable.c htable.h list.h
lasm.o: lasm.c lib.h lasm.h list.h
lib.o: lib.c lib.h
list.o: list.c list.h
lvm.o: lvm.c lib.h lvm.h lasm.h list.h htable.h
main.o: main.c lib.h lasm.h list.h lvm.h htable.h
