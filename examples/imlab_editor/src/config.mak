APPNAME = imlab
OPT=Yes

USE_AVI = Yes
#USE_WMV = Yes
USE_VIDEOCAPTURE = Yes
#USE_FFTW = Yes
USE_FFTW3 = Yes

SRCPROC = arithmetic.cpp color.cpp convolve.cpp geometric.cpp histogram.cpp \
          threshold.cpp transform.cpp render.cpp statistics.cpp \
          convolve_rank.cpp logic.cpp morphology_bin.cpp morphology_gray.cpp \
          tonegamut.cpp quantize.cpp resize.cpp effects.cpp \
          kernel.cpp unarithmetic.cpp analyze.cpp binary.cpp remotesens.cpp
SRCPROC := $(addprefix process/, $(SRCPROC))

# These does not depends on other imlab files, can be reused in other applications
SRCUTIL = im_imagematch.c im_imageview.c im_clipboard.c counter.c statusbar.c \
          utl_file.c
SRCUTIL := $(addprefix util/, $(SRCUTIL))

SRCDIALOGS = fullscreen.cpp dialogs.cpp saveas.cpp editattrib.cpp editpalette.cpp
SRCDIALOGS := $(addprefix dialogs/, $(SRCDIALOGS))

SRCWINDOWS = imagewindow.cpp mainwindow.cpp tridimwindow.cpp \
             bitmapwindow.cpp histogramwindow.cpp matrixwindow.cpp resultswindow.cpp \
             mainwindow_file.cpp mainwindow_image.cpp mainwindow_menu.cpp mainwindow_view.cpp
SRCWINDOWS := $(addprefix windows/, $(SRCWINDOWS))

SRCBASE = imagefile.cpp imlab.cpp plugin_process.cpp button_images.cpp splash.cpp \
          documentlist.cpp imagedocument.cpp counter_preview.cpp

SRC = $(SRCBASE) $(SRCWINDOWS) $(SRCDIALOGS) $(SRCUTIL) $(SRCPROC)

USE_IUP = yes
USE_CD = yes
USE_IM = yes
USE_IUPCONTROLS = Yes
USE_OPENGL = Yes
LINK_FTGL = Yes

INCLUDES := ../include . windows dialogs util process process/imp
LIBS = im_process im_jp2

ifdef USE_FFTW
  DEFINES += USE_FFTW
  LIBS += im_fftw
endif
ifdef USE_FFTW3
  DEFINES += USE_FFTW
  LIBS += im_fftw3
endif

#CPPFLAGS = -Wno-write-strings

ifneq ($(findstring Win, $(TEC_SYSNAME)), )
  SRC += imlab.rc
  
  ifneq ($(findstring vc, $(TEC_UNAME)), )
    SLIB += setargv.obj
  endif
  
  ifdef USE_AVI
    ifneq ($(findstring gcc, $(TEC_UNAME)), )
      DEFINES += USE_AVI
      LIBS += im_avi vfw_ms32 vfw_avi32
    else
      DEFINES += USE_AVI
      LIBS += im_avi vfw32
    endif
  endif
  
  ifdef USE_WMV
    ifneq ($(findstring vc, $(TEC_UNAME)), )
      DEFINES += USE_WMV
      LIBS += im_wmv wmvcore
    endif
  endif
  
  ifdef USE_VIDEOCAPTURE
    SRC += dialogs/imagecapture.cpp plugin_capture.cpp
    LIBS += strmiids im_capture
    DEFINES += USE_VIDEOCAPTURE
  endif
  
  # force the definition of math functions using float
  ifneq ($(findstring ow, $(TEC_UNAME)), )
    DEFINES += IM_DEFMATHFLOAT
  endif         
	ifneq ($(findstring bc, $(TEC_UNAME)), )
	  DEFINES += IM_DEFMATHFLOAT
	endif
  
  LIBS += iupim iupimglib iup_plot
  LIBS += cdpdf pdflib cdgl cdcontextplus gdiplus
else
  USE_STATIC = Yes
  
  ifdef USE_STATIC
    TEC_LIB_DIR := $(TEC_UNAME)
    ifdef DBG_LIB_DIR
      TEC_LIB_DIR := $(TEC_LIB_DIR)d
    endif
    
    IMLIB = $(IM)/lib/$(TEC_LIB_DIR)
    SLIB := $(addprefix $(IMLIB)/lib, $(LIBS))
    SLIB := $(addsuffix .a, $(SLIB))
    LIBS = 
    
    IUPLIB = $(IUP)/lib/$(TEC_LIB_DIR)
    SLIB += $(IUPLIB)/libiupim.a $(IUPLIB)/libiupimglib.a $(IUPLIB)/libiup_plot.a
    
    CDLIB = $(CD)/lib/$(TEC_LIB_DIR)
    SLIB += $(CDLIB)/libcdpdf.a $(CDLIB)/libpdflib.a $(CDLIB)/libcdgl.a  $(CDLIB)/libcdcontextplus.a
  else
    LIBS += iupim iupimglib iup_plot
    LIBS += cdpdf pdflib cdgl cdcontextplus
  endif

	# force the definition of math functions using float
	ifneq ($(findstring AIX, $(TEC_UNAME)), )
	  DEFINES += IM_DEFMATHFLOAT
	endif
	ifneq ($(findstring SunOS, $(TEC_UNAME)), )
	  DEFINES += IM_DEFMATHFLOAT
	endif
endif

ifdef USE_FFTW3
  ifneq ($(findstring Win, $(TEC_SYSNAME)), )
    FFTW = $(TECTOOLS_HOME)/fftw3
    ifneq ($(findstring _64, $(TEC_UNAME)), )
      LDIR = $(FFTW)/lib/Win64
    else
      LDIR = $(FFTW)/lib/Win32
    endif
    INCLUDES += $(FFTW)/include
    LIBS += libfftw3f-3 libfftw3-3
  else  
    LIBS += fftw3f fftw3
  endif
endif
