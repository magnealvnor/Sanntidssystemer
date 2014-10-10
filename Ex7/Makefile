program_EXECUTABLE := ./../sa

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

program_C_SRCS := $(wildcard *.c)
program_OBJS := ${program_C_SRCS:.c=.o}

XENO_DESTDIR:=
XENO_CONFIG:=$(XENO_DESTDIR)/usr/bin/xeno-config
XENO_CFLAGS:=$(shell DESTDIR=$(XENO_DESTDIR) $(XENO_CONFIG) --skin native --cflags)
XENO_LIBS:=$(shell DESTDIR=$(XENO_DESTDIR) $(XENO_CONFIG) --skin native --ldflags)

program_INCLUDE_DIRS := 
program_LIBRARY_DIRS :=
program_LIBRARIES :=  m $(XENO_LIBS)

LDFLAGS += -g
CFLAGS += -Wall -g $(XENO_CFLAGS)
CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))


.PHONY: all clean distclean
all: $(program_EXECUTABLE)

$(program_OBJS):%.o:%.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

$(program_EXECUTABLE): $(program_OBJS)
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) $(program_OBJS) -o $(program_EXECUTABLE) $(LDFLAGS)

clean:
	@- $(RM) $(program_EXECUTABLE)
	@- $(RM) $(program_OBJS)

distclean: clean

