CFLAGS=-march=native -Wall -Wunused -DLEXICAL_SCOPING -DARM_HAS_DIV

all: test testset scoping

parsta: src/parsta.c src/emit.c src/emit_$(shell uname -s)_$(shell uname -m).c
	gcc $(CFLAGS) $^ -o $@

%.s: %.pasta parsta
	./parsta $< $@

test: test.s src/primitives.c
	gcc $(CFLAGS) -Os test.s src/primitives.c -o test

testset: testset.s src/primitives.c
	gcc $(CFLAGS) -Os testset.s src/primitives.c -o testset

scoping: scoping.s src/primitives.c
	gcc $(CFLAGS) -Os $^ -o $@

clean:
	rm -f parsta test test.s testset testset.s scoping scoping.s
