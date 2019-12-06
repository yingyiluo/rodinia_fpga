include ../../common/make.config

COMMON_SRC = $(wildcard ../common/*.cpp)
OPENCL_INC += -I. -I../common
NAME = nn
ifeq ($(OS),Windows_NT)
	EXE = $(NAME).exe
else
	EXE = $(NAME)
endif
SRC = nearestNeighbor.cpp clutils.cpp utils.cpp

VERSIONS = 0 1 2 3 4 5
KERNEL_NAMES = $(foreach var,$(VERSIONS),$(NAME)_kernel_v$(var))
KERNEL_ALTERA_NAMES = $(addsuffix .aocx, $(KERNEL_NAMES))

ifdef ALTERA
	ifdef HOST_ONLY
		AOT_PROGRAMS = 
	else
		AOT_PROGRAMS = $(KERNEL_ALTERA_NAMES)
	endif
else
	AOT_PROGRAMS =
endif

ifdef ARM
	EXTRA_FLAGS =
else ifeq ($(OS),Windows_NT)
	EXTRA_FLAGS =
else
	EXTRA_FLAGS = -fopenmp
endif

release: $(SRC) $(AOT_PROGRAMS)
	$(CXX) $(CXXFLAGS) $(SRC) $(COMMON_SRC) -o $(EXE) $(OPENCL_INC) $(OPENCL_LIB) $(EXTRA_FLAGS)

clean: 
	$(RM) $(EXE) *.aoco $(KERNEL_NAMES)
