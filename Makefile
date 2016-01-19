CC = gcc
CXX = g++
CFLAGS = -Wall -g
CXXFLAGS = -Wall -g

OUTPUTDIR = ./bin
MKDIR_OUT = mkdir -p $(OUTPUTDIR)
OBJDIR = ./obj
MKDIR_OBJ = mkdir -p $(OBJDIR)


all: $(OUTPUTDIR)/faint $(OUTPUTDIR)/test $(OUTPUTDIR)/testcpp

$(OBJDIR):
	$(MKDIR_OUT)
	$(MKDIR_OBJ)

$(OUTPUTDIR)/faint: $(OBJDIR) $(OBJDIR)/faint.o map.c $(OBJDIR)/fault_inject 
	$(CC) $(CFLAGS) -c map.c -o $(OBJDIR)/map_c.o
	cd $(OBJDIR); $(CC) faint.o map_c.o $(CFLAGS) -Wl,--format=binary -Wl,fault_inject.so -Wl,--format=binary -Wl,fault_inject32.so -Wl,--format=default -o faint
	mv $(OBJDIR)/faint $(OUTPUTDIR)/faint

$(OBJDIR)/faint.o: faint.c
	$(CC) -c faint.c $(CFLAGS) -fno-builtin-log -o $(OBJDIR)/faint.o

$(OBJDIR)/fault_inject: fault_inject.cpp $(OBJDIR)/map.o $(OBJDIR)/map32.o
	$(CXX) $(CXXFLAGS) -fPIC -DPIC -c -fno-stack-protector -funwind-tables -fpermissive fault_inject.cpp -o $(OBJDIR)/fault_inject.o
	$(CXX) $(CXXFLAGS) -shared -o $(OBJDIR)/fault_inject.so $(OBJDIR)/map.o $(OBJDIR)/fault_inject.o -ldl

	$(CXX) $(CXXFLAGS) -fPIC -DPIC -c -fno-stack-protector -funwind-tables -fpermissive -m32 fault_inject.cpp -o $(OBJDIR)/fault_inject32.o
	$(CXX) $(CXXFLAGS) -shared -m32 -o $(OBJDIR)/fault_inject32.so $(OBJDIR)/map32.o $(OBJDIR)/fault_inject32.o -ldl
	
$(OBJDIR)/map.o: map.c
	$(CXX) $(CXXFLAGS) map.c -fPIC -DPIC -c -o $(OBJDIR)/map.o

$(OBJDIR)/map32.o: map.c
	$(CXX) $(CXXFLAGS) map.c -fPIC -DPIC -c -m32 -o $(OBJDIR)/map32.o
		
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
	