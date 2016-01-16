all: oomt test

oomt: oomt.o map.c malloc_replace 
	gcc -Wall -g -c map.c -o map_c.o
	gcc oomt.o map_c.o -Wall -g -o oomt

oomt.o: oomt.c
	gcc -c oomt.c -Wall -g -o oomt.o

malloc_replace: malloc_replace.cpp map.o
	g++ -Wall -fPIC -DPIC -c -g -fno-stack-protector -funwind-tables malloc_replace.cpp
	g++ -shared -g -o malloc_replace.so map.o malloc_replace.o -ldl

map.o: map.c
	g++ map.c -fPIC -DPIC -Wall -c -g -o map.o
	ar rcs libcmap.a map.o
	
test: test.c
	gcc test.c -Wall -g -o test
	
clean:
	-rm -f *.so *.o oomt test test_crash mallocs libcmap.a
	
run: oomt test
	./oomt test
	