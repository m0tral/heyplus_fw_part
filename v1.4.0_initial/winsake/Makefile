# Possible Targets:	all clean Debug cleanDebug Release cleanRelease

##############################################################################################
# Settings
#

# General settings
	# See $(GFXLIB)/tools/gmake_scripts/readme.txt for the list of variables
	# For Win32 this variable can be set to "win32" (native win32 api), "win32.chibios" (ChibiOS simulator),  "win32.raw32" (RAW32 simulator). 
	OPT_OS					= win32
	OPT_LINK_OPTIMIZE		= no

# uGFX settings
	# See $(GFXLIB)/tools/gmake_scripts/library_ugfx.mk for the list of variables
	GFXLIB					= ../thirdparty/ugfx
	GFXBOARD				= Win32
	#GFXDEMO				= modules/gwin/widgets
	GFXSINGLEMAKE			= no

# ChibiOS settings
ifeq ($(OPT_OS), win32.chibios)
	# See $(GFXLIB)/tools/gmake_scripts/os_win32.chibios.mk for the list of variables
	CHIBIOS					= ../ChibiOS
endif

# Win32 settings
	# See $(GFXLIB)/tools/gmake_scripts/os_win32.mk for the list of variables

##############################################################################################
# Set these for your project
#

ARCH     = i686-w64-mingw32-
SRCFLAGS = -ggdb -O0
CFLAGS   += -D _PRE_BUILD_DEF
CXXFLAGS =
ASFLAGS  =
LDFLAGS  =

SRC      = ../subsystem/gui/src/gui.c main_win.c
OBJS     =
DEFS     = 
LIBS     =
INCPATH  =	../subsystem/gui/inc
INCPATH  +=	../resources/fonts
INCPATH  +=	../resources/img
LIBPATH  =

##############################################################################################
# These should be at the end
include $(GFXLIB)/tools/gmake_scripts/library_ugfx.mk
include $(GFXLIB)/tools/gmake_scripts/os_$(OPT_OS).mk
include $(GFXLIB)/tools/gmake_scripts/compiler_gcc.mk
# *** EOF ***
