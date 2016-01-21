VERSION = 0.2

CC = gcc
CXX = g++
CFLAGS = -Wall -g -DVERSION="\"$(VERSION)\"" 
CXXFLAGS = -Wall -g -DVERSION="\"$(VERSION)\"" 

OUTPUTDIR = ./bin
OBJDIR = ./obj
SRCDIR = ./src

MKDIR_OBJ = mkdir -p $(OBJDIR)
MKDIR_OUT = mkdir -p $(OUTPUTDIR)

all: $(OUTPUTDIR)/faint

$(OBJDIR):
	$(MKDIR_OUT)
	$(MKDIR_OBJ)

$(OUTPUTDIR)/faint: $(OBJDIR) $(OBJDIR)/faint.o $(SRCDIR)/map.c $(OBJDIR)/fault_inject 
	$(CC) $(CFLAGS) -O2 -c $(SRCDIR)/map.c -o $(OBJDIR)/map_c.o
	cd $(OBJDIR); $(CC) -O2 faint.o map_c.o $(CFLAGS) -Wl,--format=binary -Wl,fault_inject.so -Wl,--format=binary -Wl,fault_inject32.so -Wl,--format=default -o faint
	mv $(OBJDIR)/faint $(OUTPUTDIR)/faint

$(OBJDIR)/faint.o: $(SRCDIR)/faint.c
	$(CC) -c $(SRCDIR)/faint.c -O2 $(CFLAGS) -Wunused-result -fno-builtin-log -o $(OBJDIR)/faint.o

$(OBJDIR)/fault_inject: $(SRCDIR)/fault_inject.cpp $(OBJDIR)/map.o $(OBJDIR)/map32.o
	$(CXX) $(CXXFLAGS) -O0 -fPIC -DPIC -c -fno-stack-protector -funwind-tables -fpermissive $(SRCDIR)/fault_inject.cpp -o $(OBJDIR)/fault_inject.o
	$(CXX) $(CXXFLAGS) -O0 -shared -o $(OBJDIR)/fault_inject.so $(OBJDIR)/map.o $(OBJDIR)/fault_inject.o -ldl

	$(CXX) $(CXXFLAGS) -O0 -fPIC -DPIC -c -fno-stack-protector -funwind-tables -fpermissive -m32 $(SRCDIR)/fault_inject.cpp -o $(OBJDIR)/fault_inject32.o
	$(CXX) $(CXXFLAGS) -O0 -shared -m32 -o $(OBJDIR)/fault_inject32.so $(OBJDIR)/map32.o $(OBJDIR)/fault_inject32.o -ldl
	
$(OBJDIR)/map.o: $(SRCDIR)/map.c
	$(CXX) $(CXXFLAGS) -O2 $(SRCDIR)/map.c -fPIC -DPIC -c -o $(OBJDIR)/map.o

$(OBJDIR)/map32.o: $(SRCDIR)/map.c
	$(CXX) $(CXXFLAGS) -O2 $(SRCDIR)/map.c -fPIC -DPIC -c -m32 -o $(OBJDIR)/map32.o
		
$(OUTPUTDIR)/test: $(SRCDIR)/test.c
	$(CC) $(CFLAGS) $(SRCDIR)/test.c -o $(OUTPUTDIR)/test
	
$(OUTPUTDIR)/test32: $(SRCDIR)/test.c
	$(CC) $(CFLAGS) $(SRCDIR)/test.c -m32 -o $(OUTPUTDIR)/test32
	
$(OUTPUTDIR)/testcpp: $(SRCDIR)/test.cpp
	$(CXX) $(CXXFLAGS) $(SRCDIR)/test.cpp -o $(OUTPUTDIR)/testcpp
	
clean:
	-rm -rf $(OUTPUTDIR) $(OBJDIR)
	
deb: $(OUTPUTDIR)/faint
	-if test `whoami` != "root"; then echo "\n\nYou need to run this target using fakeroot: fakeroot -u make deb\n"; exit 1; fi
	mkdir -p faint_$(VERSION)
	mkdir -p faint_$(VERSION)/usr
	mkdir -p faint_$(VERSION)/usr/bin
	mkdir -p faint_$(VERSION)/usr/share/doc/faint	
	cp $(OUTPUTDIR)/faint faint_$(VERSION)/usr/bin
	strip faint_$(VERSION)/usr/bin/faint
	mkdir -p faint_$(VERSION)/DEBIAN
	sed "s/%VERSION%/$(VERSION)/" debian-control > faint_$(VERSION)/DEBIAN/control
	cp copyright faint_$(VERSION)/usr/share/doc/faint/
	gzip -c -9 changelog > faint_$(VERSION)/usr/share/doc/faint/changelog.gz
	chmod -R 0755 faint_$(VERSION)/usr
	chmod 0644 faint_$(VERSION)/usr/share/doc/faint/copyright
	chmod 0644 faint_$(VERSION)/usr/share/doc/faint/changelog.gz
	chown -R root:root faint_$(VERSION)/
	dpkg-deb --build faint_$(VERSION)
	rm -rf faint_$(VERSION)
	mv faint_$(VERSION).deb $(OUTPUTDIR)
	lintian $(OUTPUTDIR)/faint_$(VERSION).deb
	
	
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
	