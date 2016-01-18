all: faint test testcpp

faint: faint.o map.c fault_inject 
	gcc -Wall -g -c map.c -o map_c.o
	gcc faint.o map_c.o -Wall -g -Wl,--format=binary -Wl,fault_inject.so -Wl,--format=default -o faint

faint.o: faint.c
	gcc -c faint.c -Wall -g -fno-builtin-log -o faint.o

fault_inject: fault_inject.cpp map.o
	g++ -Wall -fPIC -DPIC -c -g -fno-stack-protector -funwind-tables -fpermissive fault_inject.cpp
	g++ -shared -g -o fault_inject.so map.o fault_inject.o -ldl

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
	
run-io: faint test
	./faint --enable fopen --enable getline --enable fgets test
	
install: faint
	cp faint /usr/bin/faint
	
uninstall: 
	rm /usr/bin/faint
	