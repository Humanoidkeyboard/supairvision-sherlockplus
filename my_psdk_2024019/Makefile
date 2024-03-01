ARCH ?= aarch64
cflags = -fpermissive -w
CURRENT_DIR := $(CURDIR)
src = $(wildcard */)
INCLUDE := $(addprefix -I$(CURRENT_DIR)/,$(src))
INCLUDE += $(addprefix -I$(CURRENT_DIR)/,sdk/psdk_lib/include/)
INCLUDE += $(addprefix -I$(CURRENT_DIR)/,sdk/sl_lib/include/)
objdir := $(addprefix $(CURRENT_DIR)/,objs)
sourcefile = $(shell find . -type f -name "*.cpp")
objs = $(patsubst %.cpp,$(objdir)/%.o,$(notdir $(sourcefile)))
ifeq ($(ARCH),x86)
CXX = g++
CC = gcc
libsdir := $(addprefix -L$(CURRENT_DIR)/,sdk/psdk_lib/lib/x86_64-linux-gnu-gcc/)
libsdir += $(addprefix -L$(CURRENT_DIR)/,sdk/sl_lib/lib/x86_64-linux-gnu/)
else ifeq ($(ARCH),aarch64)
CXX = aarch64-linux-gnu-g++
CC = aarch64-linux-gnu-gcc
libsdir := $(addprefix -L$(CURRENT_DIR)/,sdk/psdk_lib/lib/aarch64-linux-gnu-gcc/)
libsdir += $(addprefix -L$(CURRENT_DIR)/,sdk/sl_lib/lib/aarch64-linux-gnu/)
endif

sherlockplus : $(objs) FORCE
	@$(CXX) $(libsdir) -o $@ $(addprefix $(objdir)/,$(notdir $(objs))) \
	-Wl,-Bstatic -lpayloadsdk -lsl_lidar_sdk -Wl,-Bdynamic -lpthread

$(objs) : FORCE 
	@$(CXX) $(INCLUDE) -I$(dir $(shell find -name "$(patsubst %.o,%.cpp,$(notdir $@))")) $(cflags) -c -o \
	$(objdir)/$(notdir $@) $(shell find -name "$(patsubst %.o,%.cpp,$(notdir $@))")

FORCE :

clean:
	@rm $(objs) sherlockplus

.PHONY : clean FORCE