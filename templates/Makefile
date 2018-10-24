CXXFLAGS=-g

all: replay

replay.o: replay.hh replay.cc Makefile
	g++ ${CXXFLAGS} -o replay.o -c replay.cc

draw.o: replay.hh draw.cc Makefile
	g++ ${CXXFLAGS} -o draw.o -c draw.cc

replay: replay.o draw.o Makefile
	g++ ${CXXFLAGS} -o replay replay.o draw.o -lepoxy -lwaffle-1
