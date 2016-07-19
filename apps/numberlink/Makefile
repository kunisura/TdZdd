# TdZdd: a Top-down/Breadth-first Decision Diagram Manipulation Framework
# by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
# Copyright (c) 2014 ERATO MINATO Project
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

TARGET   = ddnumlin
CXX      = g++
CPP      = $(CXX) -E
CPPFLAGS += -I../../include
CXXFLAGS += -fopenmp -Wall -fmessage-length=0
LDFLAGS  += -fopenmp
LDLIBS   +=

ifeq ($(OS),Windows_NT)
	LDLIBS += -lpsapi
endif

SRCS     = $(wildcard *.cpp)
OBJS     = $(SRCS:%.cpp=%.o)

.PONY: all debug clean depend

all:	$(TARGET)
debug:	$(TARGET:%=%-debug)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -O3 $(CPPFLAGS) -DNDEBUG $(TARGET_ARCH) -c -o $@ $<

%-debug.o: %.cpp
	$(CXX) $(CXXFLAGS) -g $(CPPFLAGS) -DDEBUG $(TARGET_ARCH) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -o $@

$(TARGET:%=%-debug): $(OBJS:%.o=%-debug.o)
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -o $@

clean:
	$(RM) $(TARGET) $(OBJS) $(TARGET:%=%-debug) $(OBJS:%.o=%-debug.o)

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
endef

depend:
	$(make-depend)

depend.in: $(SRCS) $(HDRS) Makefile
	$(make-depend)

include depend.in
