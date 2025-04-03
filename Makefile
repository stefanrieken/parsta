all: test testset

parsta: src/parsta.c src/emit.c src/emit_$(shell uname -s)_$(shell uname -m).c
	gcc -Wall -Wunused $^ -o $@

%.s: %.pasta parsta
	./parsta $< $@

test: test.s src/primitives.c
	gcc -Os test.s src/primitives.c -o test

testset: testset.s src/primitives.c
	gcc -Os testset.s src/primitives.c -o testset

clean:
	rm -f parsta test test.s testset testset.s
