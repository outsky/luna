# Autogened at 2019/08/27 13:42:39

BIN = luna

CFLAGS = -g -Wall -std=c99 -D_GNU_SOURCE

LIBS = #empty

ALL_O = lasm.o lib.o list.o main.o 

$(BIN): $(ALL_O)
	cc -o $@ $(CFLAGS) $(ALL_O) $(LIBS)

clean:
	rm -f $(BIN) $(ALL_O)

# autogen with cc -MM
lasm.o: lasm.c lasm.h
lib.o: lib.c lib.h
list.o: list.c list.h
main.o: main.c lib.h
