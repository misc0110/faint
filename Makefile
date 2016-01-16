all: oomt test

oomt: oomt.c malloc_replace 
	gcc oomt.c -Wall -gdwarf -o oomt

malloc_replace: malloc_replace.cpp map.o
	g++ -Wall -fPIC -DPIC -c -g -fno-stack-protector -funwind-tables malloc_replace.cpp
	g++ -shared -g -o malloc_replace.so map.o malloc_replace.o -ldl

map.o: map.c
	g++ map.c -fPIC -DPIC -Wall -c -g -o map.o
	ar rcs libcmap.a map.o
	
test: test.c
	gcc test.c -Wall -funwind-tables -o test
	
clean:
	-rm -f *.so *.o oomt test test_crash mallocs libcmap.a
	
run: oomt test
	./oomt test
	