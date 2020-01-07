# *** ENTER THE TARGET NAME HERE ***
TARGET      = TrackAIr

# important directories used by assorted rules and other variables
DIR_OBJS   = .objects

# compiler options
CC          = gcc
CX          = g++
CCFLAGS     = -O2 -O3 -DLINUX -D_GNU_SOURCE -Wall $(INCLUDES) $(FORMAT) -g
CXFLAGS     = -O2 -O3 -DLINUX -D_GNU_SOURCE -Wall $(INCLUDES) $(FORMAT) -g
LNKCC       = $(CX)
LNKFLAGS    = $(CXFLAGS) #-Wl,-rpath,$(DIR_THOR)/lib
FORMAT      = -m64

OPENCV := $(shell pkg-config opencv4 --libs --cflags)

#---------------------------------------------------------------------
# Core components (all of these are likely going to be needed)
#---------------------------------------------------------------------
INCLUDES   += -I/home/theb3arybear/Desktop/TrackAIr/Program/DynamixelSDK/c++/include/dynamixel_sdk -I/home/theb3arybear/Desktop/TrackAIr/Program/opencv4 -I/home/theb3arybear/Desktop/TrackAIr/Program/fltk-1.3.5

LIBRARIES  += -ldxl_x64_cpp
LIBRARIES  += -lrt
LIBRARIES  += -lpthread
LIBRARIES	 += $(OPENCV)
LIBRARIES  += -L/home/theb3arybear/Desktop/TrackAIr/Program/fltk-1.3.5/lib -lfltk -lX11 -lXfixes -lXext -ldl -lXrender -lXcursor -lfontconfig -lXft -lXinerama


#---------------------------------------------------------------------
# Files
#---------------------------------------------------------------------
SOURCES = read_write.cpp \
    # *** OTHER SOURCES GO HERE ***

OBJECTS  = $(addsuffix .o,$(addprefix $(DIR_OBJS)/,$(basename $(notdir $(SOURCES)))))
#OBJETCS += *** ADDITIONAL STATIC LIBRARIES GO HERE ***


#---------------------------------------------------------------------
# Compiling Rules
#---------------------------------------------------------------------
$(TARGET): make_directory $(OBJECTS)
	$(LNKCC) $(CFLAGS) $(LNKFLAGS) $(LDFLAGS) $(OBJECTS) -o $(TARGET) $(LIBRARIES)

all: $(TARGET)

clean:
	rm -rf $(TARGET) $(DIR_OBJS) core *~ *.a *.so *.lo

make_directory:
	mkdir -p $(DIR_OBJS)/

$(DIR_OBJS)/%.o: ../%.c
	$(CC) $(CCFLAGS) -c $? -o $@

$(DIR_OBJS)/%.o: ../%.cpp
	$(CX) $(CXFLAGS) -c $? -o $@

#---------------------------------------------------------------------
# End of Makefile
#---------------------------------------------------------------------
