# Makefile

TARGET	= ddpaths ddqueens
EXTRA	= cnfbdd cnf2ztdd2bdd graphillion_example test 
BINDIR	= bin debug

SRCS	= $(wildcard */*.cpp)
HDRS	= $(wildcard */*.hpp ../include/*/*.hpp ../include/*/*/*.hpp)
OBJS	= $(SRCS:%.cpp=%.o) $(SRCS:%.cpp=%-debug.o)

CXX	= g++
CPP	= $(CXX) -E

CPPFLAGS = -I../include
CXXFLAGS = -fopenmp -Wall -fmessage-length=0
LDFLAGS	 = -fopenmp

%/cnfbdd:		LDLIBS += -lcudd
%/cnf2ztdd2bdd:		LDLIBS += -lcudd
%/graphillion_example:	LDLIBS += -lsapporo
%/test:			LDLIBS += -lgtest

.PONY: apps all debug test clean depend

apps:	$(TARGET:%=bin/%)
all:	$(TARGET:%=bin/%) $(EXTRA:%=bin/%)
debug:	$(TARGET:%=debug/%) $(EXTRA:%=debug/%)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -O3 $(CPPFLAGS) -DNDEBUG $(TARGET_ARCH) -c -o $@ $<

%-debug.o: %.cpp
	$(CXX) $(CXXFLAGS) -g $(CPPFLAGS) -DDEBUG $(TARGET_ARCH) -c -o $@ $<

bin/%:
	@ mkdir -pv $(dir $@)
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -o $@

debug/%:
	@ mkdir -pv $(dir $@)
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -o $@

test: debug/test
	debug/test

clean:
	rm -rf $(BINDIR) $(OBJS)

define make-depend
	$(RM) depend.in
	for i in $(SRCS:%.cpp=%); do\
	    $(CPP) $(CPPFLAGS) -MM $$i.cpp | perl -n0 -e\
	        's!\s+(?:\\\s*)?\S*\.cpp!!g;\
	         s!^\S+.o:!'$$i'.o:!;\
	         print;\
	         s!^\S+.o:!'$$i'-debug.o:!;\
	         print' >> depend.in;\
	done
	for i in $(TARGET) $(EXTRA); do\
	    perl -e'\
	        my $$dir = (<*/'$$i'.cpp>)[0];\
	        $$dir =~ s/\/.*//;\
	        print "bin/'$$i': ",\
	            join(" ", grep(s/\.cpp$$/.o/, <$$dir/*.cpp>)), "\n";\
	        print "debug/'$$i': ",\
	            join(" ", grep(s/\.cpp$$/-debug.o/, <$$dir/*.cpp>)), "\n";\
	    ' >> depend.in;\
	done
endef

depend:
	$(make-depend)

depend.in: $(SRCS) $(HDRS) Makefile
	$(make-depend)

include depend.in
