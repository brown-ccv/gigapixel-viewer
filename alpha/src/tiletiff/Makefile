#
# CAVE Makefile
#
TARGET = ../../bin/tiletiff

CC = gcc -g
CXX = g++ -g


#
# Object files
#
#OBJS = main.o  matrix.o  quads.o shape_layer.o topo_layer.o shpopen.o $(SPACEBALL_OBJS)

#
# User CFlags
#
#USERCFLAGS = -I/usr/local/libsball -I/usr/local/liborb -Imath
#
# Libraries
#
# General stuff
GENLIBS		= -ltiff

# Combine all includes and libraries
ALLLIBS         = -Lmath -L/usr/X11R6/lib $(GENLIBS)

#
# Set up compiler and loader flags
#
CFLAGS = $(USERCFLAGS)
CXXFLAGS = $(CFLAGS)
#LFLAGS   = -O

all: $(TARGET) ../../bin/fill_holes 

../../bin/tiletiff: tiletiff.o main.o
	$(CC) tiletiff.c main.c $(LFLAGS) -o $@  $(ALLLIBS)
../../bin/fill_holes: tiletiff.o fill_holes.o
	$(CC) tiletiff.c fill_holes.c $(LFLAGS) -o $@  $(ALLLIBS)

.SUFFIXES: .cxx
.cxx.o:
	$(CXX) $(CXXFLAGS) -c $<

clean: 
	rm -rf *.o 
	rm -rf core
	rm -f ../../bin/fill_holes
	rm -f ../../bin/tiffre
	rm -f ../../bin/tiletiff
	rm -f ../../bin/tiffchunk

clobber: clean
	rm $(TARGETS)
