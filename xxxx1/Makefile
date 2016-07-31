all: Roman

Roman: cutcheck.o cut-pass.o cutgen.o Methods.o
	gcc cutcheck.o cut-pass.o cutgen.o Methods.o -o Roman

cutcheck.o: cutcheck.c
	gcc -c cutcheck.c

Methods.o: Methods.c
	gcc -c Methods.c

cut-pass.o: cut-pass.c Methods.h
	gcc -c cut-pass.c

cutgen.o: cutgen.c
	gcc -c cutgen.c

clean:
	rm -rf *o Roman
