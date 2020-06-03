#
# Makefile
#
.DEFAULT: default

OBJS=JDebug.o JDiff.o JFileAhead.o JFileIStream.o JFileIStreamAhead.o JHashPos.o JMatchTable.o JOutAsc.o JOutBin.o JOutRgn.o main.o 

default:	linux
all: 		linux 
full:		clean linux
linux:		jdiff jptch
debug:		jdifd jptcd
mingw:		jdiff.exe jptch.exe 

CC=gcc
CPP=g++
CFLAGS=-Os -Wall
NATIVE=-march=native

debug:DBG=-g -D_DEBUG
#linux:CFLAGS+=-D_FILE_OFFSET_BITS=64 -U_LARGEFILE64_SOURCE
mingw:CFLAGS+=-D_LARGEFILE64_SOURCE
mingw:LDFLAGS=-static

%.o: %.cpp
	$(CPP) $(CFLAGS) $(DBG) -o $@ -c $<

clean:
	rm -f jdiff jptch jdifd jptcd *.exe $(OBJS)

jdiff: $(OBJS)
	$(CPP) $(CFLAGS) $(DBG) $(OBJS) -o jdiff

jptch: jpatch.cpp
	$(CPP) $(CFLAGS) $(DBG) -o jptch jpatch.cpp

jdifd: $(OBJS)
	$(CPP) $(CFLAGS) $(DBG) $(OBJS) -o jdifd

jptcd: jpatch.cpp
	$(CPP) $(CFLAGS) $(DBG) -o jptcd jpatch.cpp

jdiff.exe: $(OBJS)
	$(CPP) $(CFLAGS) $(LDFLAGS) $(DBG) $(OBJS) -o jdiff.exe

jptch.exe: jpatch.cpp
	$(CPP) $(CFLAGS) $(LDFLAGS) $(DBG) -o jptch.exe jpatch.cpp 

