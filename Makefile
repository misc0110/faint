CC = gcc
CXX = g++
CFLAGS = -Wall -g
CXXFLAGS = -Wall -g

OUTPUTDIR = ./bin
MKDIR_OUT = mkdir -p $(OUTPUTDIR)
OBJDIR = ./obj
MKDIR_OBJ = mkdir -p $(OBJDIR)


all: $(OUTPUTDIR)/faint

$(OBJDIR):
	$(MKDIR_OUT)
	$(MKDIR_OBJ)

$(OUTPUTDIR)/faint: $(OBJDIR) $(OBJDIR)/faint.o map.c $(OBJDIR)/fault_inject 
	$(CC) $(CFLAGS) -O2 -c map.c -o $(OBJDIR)/map_c.o
	cd $(OBJDIR); $(CC) -O2 faint.o map_c.o $(CFLAGS) -Wl,--format=binary -Wl,fault_inject.so -Wl,--format=binary -Wl,fault_inject32.so -Wl,--format=default -o faint
	mv $(OBJDIR)/faint $(OUTPUTDIR)/faint

$(OBJDIR)/faint.o: faint.c
	$(CC) -c faint.c -O2 $(CFLAGS) -Wunused-result -fno-builtin-log -o $(OBJDIR)/faint.o

$(OBJDIR)/fault_inject: fault_inject.cpp $(OBJDIR)/map.o $(OBJDIR)/map32.o
	$(CXX) $(CXXFLAGS) -O0 -fPIC -DPIC -c -fno-stack-protector -funwind-tables -fpermissive fault_inject.cpp -o $(OBJDIR)/fault_inject.o
	$(CXX) $(CXXFLAGS) -O0 -shared -o $(OBJDIR)/fault_inject.so $(OBJDIR)/map.o $(OBJDIR)/fault_inject.o -ldl

	$(CXX) $(CXXFLAGS) -O0 -fPIC -DPIC -c -fno-stack-protector -funwind-tables -fpermissive -m32 fault_inject.cpp -o $(OBJDIR)/fault_inject32.o
	$(CXX) $(CXXFLAGS) -O0 -shared -m32 -o $(OBJDIR)/fault_inject32.so $(OBJDIR)/map32.o $(OBJDIR)/fault_inject32.o -ldl
	
$(OBJDIR)/map.o: map.c
	$(CXX) $(CXXFLAGS) -O2 map.c -fPIC -DPIC -c -o $(OBJDIR)/map.o

$(OBJDIR)/map32.o: map.c
	$(CXX) $(CXXFLAGS) -O2 map.c -fPIC -DPIC -c -m32 -o $(OBJDIR)/map32.o
		
$(OUTPUTDIR)/test: test.c
	$(CC) $(CFLAGS) test.c -o $(OUTPUTDIR)/test
	
$(OUTPUTDIR)/test32: test.c
	$(CC) $(CFLAGS) test.c -m32 -o $(OUTPUTDIR)/test32
	
$(OUTPUTDIR)/testcpp: test.cpp
	$(CXX) $(CXXFLAGS) test.cpp -o $(OUTPUTDIR)/testcpp
	
clean:
	-rm -rf $(OUTPUTDIR) $(OBJDIR)
	
run: $(OUTPUTDIR)/faint $(OUTPUTDIR)/test
	$(OUTPUTDIR)/faint $(OUTPUTDIR)/test
	
runcpp: $(OUTPUTDIR)/faint $(OUTPUTDIR)/testcpp
	$(OUTPUTDIR)/faint $(OUTPUTDIR)/testcpp
	
run32: $(OUTPUTDIR)/faint $(OUTPUTDIR)/test32
	$(OUTPUTDIR)/faint $(OUTPUTDIR)/test32
	
run-io: $(OUTPUTDIR)/faint $(OUTPUTDIR)/test
	$(OUTPUTDIR)/faint --no-memory --file-io $(OUTPUTDIR)/test
	
install: $(OUTPUTDIR)/faint
	cp $(OUTPUTDIR)/faint /usr/bin/faint
	
uninstall: 
	rm /usr/bin/faint
	