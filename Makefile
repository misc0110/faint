all: faint test testcpp

faint: faint.o map.c malloc_replace 
	gcc -Wall -g -c map.c -o map_c.o
	gcc faint.o map_c.o -Wall -g -o faint

faint.o: faint.c
	gcc -c faint.c -Wall -g -fno-builtin-log -o faint.o

malloc_replace: malloc_replace.cpp map.o
	g++ -Wall -fPIC -DPIC -c -g -fno-stack-protector -funwind-tables malloc_replace.cpp
	g++ -shared -g -o malloc_replace.so map.o malloc_replace.o -ldl

map.o: map.c
	g++ map.c -fPIC -DPIC -Wall -c -g -o map.o
	
test: test.c
	gcc test.c -Wall -g -o test
	
testcpp: test.cpp
	g++ test.cpp -Wall -g -o testcpp
	
clean:
	-rm -f *.so *.o faint test mallocs profile settings testcpp
	
run: faint test
	./faint test
	
runcpp: faint testcpp
	./faint testcpp
	