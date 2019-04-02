# =====
# Makefile used for testing changes from Linux.
# =====

# Set compiler to mingw (can still override from command line)
CXX := i686-w64-mingw32-g++

SRCDIR := .
BUILDDIR := .build
BINDIR := $(BUILDDIR)/bin
OBJDIR := $(BUILDDIR)/obj
DEPDIR := $(BUILDDIR)/obj
OUTPUT := NetFix.dll

CPPFLAGS := -I OP2Internal/src/ -I op2ext/srcDLL/
CXXFLAGS := -std=c++17 -g -Wall -Wno-unknown-pragmas -Wzero-as-null-pointer-constant
LDFLAGS := -shared -LOP2Internal/
LDLIBS := -lOP2Internal -lws2_32

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.cpp = $(CXX) $(DEPFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

SRCS := $(shell find $(SRCDIR) -maxdepth 1 -name '*.cpp')
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
FOLDERS := $(sort $(dir $(SRCS)))

.PHONY: default all op2internal
# default: all
# Project does not link correctly on Linux due to name mangling differences
# For now, default build rules stop before the link step
default: $(OBJS) | op2internal
all: $(OUTPUT)

op2internal:
	make -C OP2Internal/
$(OUTPUT): | op2internal

$(OUTPUT): $(OBJS)
	@mkdir -p ${@D}
	$(CXX) $^ $(LDFLAGS) -o $@ $(LDLIBS)

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp $(DEPDIR)/%.d | build-folder
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

.PHONY: build-folder
build-folder:
	@mkdir -p $(patsubst $(SRCDIR)/%,$(OBJDIR)/%, $(FOLDERS))
	@mkdir -p $(patsubst $(SRCDIR)/%,$(DEPDIR)/%, $(FOLDERS))

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst $(SRCDIR)/%.cpp,$(DEPDIR)/%.d,$(SRCS)))

.PHONY: clean clean-deps clean-all
clean:
	-rm -fr $(OBJDIR)
	-rm -fr $(DEPDIR)
	-rm -fr $(BINDIR)
	-rm -f $(OUTPUT)
clean-deps:
	-rm -fr $(DEPDIR)
clean-all:
	-rm -rf $(BUILDDIR)
