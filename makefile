# =====
# Makefile used for testing changes from Linux.
# =====
# Project does not link correctly on Linux due to name mangling differences
# For now, default build rules stop before the link step


include op2ext/makefile-generic.mk


# Set default compiler toolchain (gcc, clang, mingw, default)
config := default
# Client project is Windows only, and so needs a compiler that targets Windows
netFixClient: config := mingw
intermediate-netFixClient: config := mingw


CPPFLAGS := -I OP2Internal/src/ -I op2ext/srcDLL/
CXXFLAGS := -std=c++17 -g -Wall -Wno-unknown-pragmas -Wzero-as-null-pointer-constant
LDFLAGS := -shared -LOP2Internal/
LDLIBS := -lOP2Internal -lws2_32

.PHONY: all op2internal op2ext

# By default only compile intermediate files, do not link
all: intermediate-netFixClient

op2internal:
	make -C OP2Internal/

op2ext:
	make -C op2ext/ op2extDll

NetFix.dll: | op2internal op2ext

$(eval $(call DefineCppProject,netFixClient,NetFix.dll,client/))
