include ../../common/make.config

NAME = backprop

COMMON_SRC = $(wildcard ../common/*.cpp)
OPENCL_INC += -I. -I../common

ifeq ($(OS),Windows_NT)
	EXE = $(NAME).exe
else
	EXE = $(NAME)
endif
SRC = backprop_ocl.cpp backprop.c facetrain.c imagenet.c

ifdef OUTPUT
	EXTRA_FLAGS += -DOUTPUT
endif

ifdef ALTERA
	ifdef HOST_ONLY
		AOT_PROGRAMS = 
	else
		AOT_PROGRAMS = $(KERNEL_ALTERA_NAMES)
	endif
else
	AOT_PROGRAMS =
endif

EXTRA_FLAGS += -Wno-unused-result

all: $(EXE) $(AOT_PROGRAMS) $(COMMON_SRC)

profile: CFLAGS += -DPROFILE
profile: $(EXE) $(AOT_PROGRAMS) $(COMMON_SRC)

$(EXE): $(SRC)
	$(CXX) $(CFLAGS) $(SRC) -o $(EXE) $(COMMON_SRC) $(OPENCL_INC) $(OPENCL_LIB) $(EXTRA_FLAGS)

clean:
	$(RM) $(EXE) *.aoco $(KERNEL_NAMES)
