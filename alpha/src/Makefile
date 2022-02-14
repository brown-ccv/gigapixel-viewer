#
# CAVE Makefile
#
TARGET = ../bin/gigapixelviewer

CPP = g++
CPPFLAGS = -Wall -g
LDFLAGS = -O

#
# Object files
#
OBJDIR = obj

SRCS = $(shell ls *.cpp)

OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)

# Glut stuff
GLUTLIBS        = -lglut -ltiff -lpthread

# GL stuff
GLLIBS          = -lGLU -lGL

#CG stuff
#CG_INCS = -I./cg/include/
#CGLIBS = -L./cg/lib64/ -lCg -lCgGL

VRG3D_INCLUDES = -I${VRG3D_BASE}/include 
VRG3D_LIBS = -L${VRG3D_BASE}/lib -Xlinker --start-group  -Xlinker -ldl  -Xlinker -lX11 -Xlinker \
	-lXext  -Xlinker -lpthread  -Xlinker -lz  -Xlinker -ljpeg  -Xlinker \
	-lpng  -Xlinker -lzip  -Xlinker -lSDL -Xlinker -lvrg3d  -Xlinker \
	-lavutil  -Xlinker -lavformat  -Xlinker -lavcodec  -Xlinker -lGLG3D \
	-Xlinker -lG3D -Xlinker -lGLU -Xlinker -lGL -Xlinker --end-group \
	-lvrpn -lglut -lXmu -lmplEasyBlendSDK

# General stuff
GENLIBS		= -lpng -lzip

# Combine all includes and libraries
ALLINCLS        =  $(GLUTINCLS) $(VRG3D_INCLUDES)
ALLLIBS         = -Lmath -L/usr/X11R6/lib $(GLUTLIBS) $(GLLIBS) $(VRG3D_LIBS)

$(TARGET): $(OBJS)
	$(CPP) $(LDFLAGS) -o $(TARGET) $^ $(ALLLIBS)

$(OBJDIR)/%.o: %.cpp
	mkdir -p $(OBJDIR)
	$(CPP) $(CPPFLAGS) -c $< -o $@ $(ALLINCLS)

clean:
	rm -f $(TARGET) $(OBJDIR)/*.o demo-log.txt
	rm -rf $(OBJDIR)

