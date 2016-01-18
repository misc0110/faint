all: faint test testcpp

faint: faint.o map.c fault_inject 
	gcc -Wall -g -c map.c -o map_c.o
	gcc faint.o map_c.o -Wall -g -Wl,--format=binary -Wl,fault_inject.so -Wl,--format=binary -Wl,fault_inject32.so -Wl,--format=default -o faint

faint.o: faint.c
	gcc -c faint.c -Wall -g -fno-builtin-log -o faint.o

fault_inject: fault_inject.cpp map.o map32.o
	g++ -Wall -fPIC -DPIC -c -g -fno-stack-protector -funwind-tables -fpermissive fault_inject.cpp
	g++ -shared -g -o fault_inject.so map.o fault_inject.o -ldl

	g++ -Wall -fPIC -DPIC -c -g -fno-stack-protector -funwind-tables -fpermissive -m32 fault_inject.cpp -o fault_inject32.o
	g++ -shared -g -m32 -o fault_inject32.so map32.o fault_inject32.o -ldl
	
map.o: map.c
	g++ map.c -fPIC -DPIC -Wall -c -g -o map.o

map32.o: map.c
	g++ map.c -fPIC -DPIC -Wall -c -g -m32 -o map32.o
		
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
	./faint --no-memory --file-io test
	
install: faint
	cp faint /usr/bin/faint
	
uninstall: 
	rm /usr/bin/faint
	