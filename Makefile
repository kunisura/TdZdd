# $Id: Makefile 518 2014-03-18 05:29:23Z iwashita $

CONFIG	= Release Debug Prof
TARGET	= $(patsubst %.cpp,%,$(wildcard *.cpp))
#TARGET  = test1

SRCS	= $(wildcard *.cpp */*.cpp)
HDRS	= $(wildcard *.hpp */*.hpp)

CXX	= g++
CPP	= $(CXX) -E
CPPFLAGS = -I. #-std=c++11
CXXFLAGS = -Wall -fmessage-length=0
LDFLAGS	 = 

$(CONFIG:%=%/cnfbdd):	       LDLIBS += -lcudd
$(CONFIG:%=%/cnf2bdd):	       LDLIBS += -lcudd
$(CONFIG:%=%/countpaths):      LDLIBS +=
$(CONFIG:%=%/ddcnf):	       LDLIBS += -lcudd
$(CONFIG:%=%/ddpaths):	       LDLIBS +=
$(CONFIG:%=%/example):         LDLIBS += -lsapporo
$(CONFIG:%=%/graphilliontest): LDLIBS += -lsapporo
$(CONFIG:%=%/testcudd):        LDLIBS += -lcudd
devel:	CXXFLAGS += -fopenmp
devel:	LDFLAGS += -fopenmp -static
#devel:	LDLIBS += -ltcmalloc_minimal
debug:	CXXFLAGS += -fopenmp
debug:	LDFLAGS += -fopenmp
prof:	CXXFLAGS += -fopenmp -pg
prof:	LDFLAGS += -fopenmp -pg

.PONY: all devel debug prof clean depend

all:	$(TARGET:%=Release/%)
devel:	$(TARGET:%=Release/%)
debug:	$(TARGET:%=Debug/%)
prof:	$(TARGET:%=Prof/%)
clean:
	rm -rf Release Debug Prof depend.in

Release/%.o: %.cpp
	@ mkdir -pv $(dir $@)
	$(CXX) $(CXXFLAGS) -g -O3 $(CPPFLAGS) -DNDEBUG $(TARGET_ARCH) -c -o $@ $<

Debug/%.o: %.cpp
	@ mkdir -pv $(dir $@)
	$(CXX) $(CXXFLAGS) -g $(CPPFLAGS) -DDEBUG  $(TARGET_ARCH) -c -o $@ $<

Prof/%.o: %.cpp
	@ mkdir -pv $(dir $@)
	$(CXX) $(CXXFLAGS) -g -O3 $(CPPFLAGS) -DNDEBUG  $(TARGET_ARCH) -c -o $@ $<

%: %.o
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -o $@

define make-depend
	$(RM) depend.in
	for c in $(CONFIG); do\
		for i in $(SRCS:%.cpp=%); do\
			$(CPP) $(CPPFLAGS) -MM $$i.cpp | perl -p0 -e\
				's!^\S+.o:!'$$c'/'$$i'.o:!;\
			 	 s/ *[^ ]*\.cpp//g' >> depend.in;\
		done;\
		for i in $(TARGET); do\
			$(CPP) $(CPPFLAGS) -MM $$i.cpp | perl -p0 -e\
				's/^(\S+)\.o:/'$$c'\/$$1:/;\
			 	 s/ *(\S+)\.[ch]pp/-f"$$1.cpp"?" '$$c'\/$$1.o":""/eg;\
			 	 s/\n *\\\n/\n/g' >> depend.in;\
		done;\
	done
endef

depend:
	$(make-depend)

depend.in: $(SRCS) $(HDRS)
	$(make-depend)

-include depend.in
