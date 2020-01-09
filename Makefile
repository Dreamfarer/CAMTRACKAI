# *** ENTER THE TARGET NAME HERE ***
TARGET      = CamTrackAI

# important directories used by assorted rules and other variables
DIR_OBJS   = objects

# Path to CamTrackAI folder
CAMTRACKAI_PATH = /home/theb3arybear/Desktop/CamTrackAI/GitHub/CamTrackAI

# compiler options
CC          = gcc
CX          = g++
CCFLAGS     = -O2 -O3 -DLINUX -D_GNU_SOURCE -Wall $(INCLUDES) $(FORMAT) -g
CXFLAGS     = -O2 -O3 -DLINUX -D_GNU_SOURCE -Wall $(INCLUDES) $(FORMAT) -g
LNKCC       = $(CX)
LNKFLAGS    = $(CXFLAGS) #-Wl,-rpath,$(DIR_THOR)/lib
FORMAT      = -m64

#---------------------------------------------------------------------
# INCLUDE PATH - Tell the compiler where to find header (.hpp / .h) files
#---------------------------------------------------------------------
INCLUDES   += -I/usr/include/opencv4
INCLUDES   += -I$(CAMTRACKAI_PATH)/fltk
INCLUDES   += -I$(CAMTRACKAI_PATH)/DynamixelSDK/c++/include/dynamixel_sdk

#---------------------------------------------------------------------
# LIBRARY PATH - Tell the compiler where to find libraries
#---------------------------------------------------------------------
#fltk Libray
LIBRARIES  += -L$(CAMTRACKAI_PATH)/fltk/lib
LIBRARIES  += -lfltk

#OpenCV Library
LIBRARIES  += -lopencv_gapi -lopencv_stitching -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dnn_objdetect -lopencv_dnn_superres -lopencv_dpm -lopencv_highgui -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hfs -lopencv_img_hash -lopencv_line_descriptor -lopencv_quality -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow -lopencv_surface_matching -lopencv_tracking -lopencv_datasets -lopencv_text -lopencv_dnn -lopencv_plot -lopencv_videostab -lopencv_video -lopencv_videoio -lopencv_xfeatures2d -lopencv_shape -lopencv_ml -lopencv_ximgproc -lopencv_xobjdetect -lopencv_objdetect -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_flann -lopencv_xphoto -lopencv_photo -lopencv_imgproc -lopencv_core

#Dynamixel
LIBRARIES  += -ldxl_x64_cpp
#dxl_sbc_cpp
#dxl_x64_cpp

#Other
LIBRARIES  += -lrt -lpthread -lX11 -lXfixes -lXext -ldl -lXrender -lXcursor -lfontconfig -lXft -lXinerama

#---------------------------------------------------------------------
# Files
#---------------------------------------------------------------------
SOURCES = CamTrackAI.cpp \
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

$(DIR_OBJS)/%.o: %.c
	$(CC) $(CCFLAGS) -c $? -o $@

$(DIR_OBJS)/%.o: %.cpp
	$(CX) $(CXFLAGS) -c $? -o $@
