# Autogened at 2019/08/31 23:25:27

BIN = luna

CFLAGS = -g -Wall -std=c99 -D_GNU_SOURCE

LIBS = #empty

ALL_O = htable.o lasm.o list.o ltable.o luna.o lvm.o main.o 

$(BIN): $(ALL_O)
	cc -o $@ $(CFLAGS) $(ALL_O) $(LIBS)

clean:
	rm -f $(BIN) $(ALL_O)

# autogen with cc -MM
htable.o: htable.c luna.h htable.h list.h
lasm.o: lasm.c luna.h lasm.h list.h ltable.h htable.h
list.o: list.c luna.h list.h
ltable.o: ltable.c ltable.h luna.h htable.h list.h
luna.o: luna.c luna.h
lvm.o: lvm.c luna.h lvm.h lasm.h list.h ltable.h htable.h
main.o: main.c luna.h lasm.h list.h ltable.h htable.h lvm.h
