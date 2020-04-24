MKDIR_P := mkdir -p
CP := cp
MV := mv
CC := gcc
CCC := g++
RM_RF = rm -rf

# directories
CWD := $(realpath .)
BINDIR := $(CWD)/bin
BUILDDIR := $(CWD)/build
SRCDIR := $(CWD)/src
INCLUDEDIR := $(CWD)/include

# flas
CFLAGS := -O2 -I$(INCLUDEDIR) -Wall -Wextra
LDFLAGS := -pthread

# target files
DIRS_TARGET := $(BINDIR) $(BUILDDIR)
TARGET := $(BINDIR)/mailattach
SRCFILES := $(wildcard $(SRCDIR)/*.c)
OBJFILES := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCFILES))

# fancy targets
all: directories $(TARGET)

directories: $(DIRS_TARGET)

# less fancy targets

$(DIRS_TARGET):
	$(MKDIR_P) $@

$(TARGET): $(OBJFILES)
	$(CC) $(LDFLAGS) -o $@  $^ /usr/lib/libmilter.a
 
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM_RF) $(DIRS_TARGET)
