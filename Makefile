# Path to the module directory, i.e. the grass sourcecode main path.
# Relative or absolute.
MODULE_TOPDIR = ~/gis/grass-src

PGM = r.refine


# Includes the grass make-System.
include $(MODULE_TOPDIR)/include/Make/Module.make

SOURCES = main.c  rtimer.c pqelement.c pqheap.c tin.c refine_tin.c\
	grid.c queue.c geom_tin.c qsort.c render_tin.c mem_manager.c\
	grass.c
HEADERS = main.h  rtimer.h pqelement.h pqheap.h tin.h refine_tin.h\
	grid.h queue.h geom_tin.h qsort.h render_tin.h mem_manager.h\
	constants.h grass.h point.h triangle.h


OBJARCH=OBJ.$(ARCH)
OBJ := $(patsubst %.c,$(OBJARCH)/%.o,$(SOURCES))

LIBS = $(GISLIB) 
DEPLIBS = $(DEPGISLIB)
CLEAN_SUBDIRS = 


# Using GNU c++ compiler.
CXX = gcc


# Set compiler and load flags. 
CXXFLAGS += -O3 -DNO_STATS #-DNDEBUG
CXXFLAGS += $(WARNING_FLAGS) -DUSER=\"$(USER)\" \
		-D_FILE_OFFSET_BITS=64  -D_LARGEFILE_SOURCE\
		-D__GRASS__ -fmessage-length=0
CXXFLAGS += -ffast-math -funroll-loops

WARNING_FLAGS   = -Wall -Wformat  -Wparentheses -Wpointer-arith\
		-Wno-conversion -Wreturn-type	\
		-Wcomment  -Wno-sign-compare -Wno-unused
#-Wimplicit-int -Wimplicit-function-declaration

DBGLDFLAGS=

# Make rules.
# Default is the 'default' target for the grass make system.
$(OBJARCH)/%.o:%.c
	$(CXX) -c $(CXXFLAGS)  $< -o $@


default: $(BIN)/$(PGM)$(EXE)
	$(MAKE) htmlcmd

$(BIN)/$(PGM)$(EXE):  $(OBJ) $(DEPLIBS) 
	$(CXX) $(LDFLAGS) -o $@ $(OBJ) $(LIBS) $(MATHLIB) $(XDRLIB)  $(DBGLDFLAGS)

