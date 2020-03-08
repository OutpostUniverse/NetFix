# =====
# Makefile used for testing changes from Linux.
# =====
# Project does not link correctly on Linux due to name mangling differences
# For now, default build rules stop before the link step


# Capture top level folder before any Makefile includes
# Note: MAKEFILE_LIST's last entry is the last processed Makefile.
#       That should be the current Makefile, assuming no includes
TopLevelFolder := $(abspath $(dir $(lastword ${MAKEFILE_LIST})))


include op2ext/makefile-generic.mk


# Set default compiler toolchain (gcc, clang, mingw, default)
config := default
# Client project is Windows only, and so needs a compiler that targets Windows
netFixClient: config := mingw
intermediate-netFixClient: config := mingw


CXXFLAGS := -std=c++17 -g -Wall -Wno-unknown-pragmas -Wzero-as-null-pointer-constant

netFixClient_CPPFLAGS := -I OP2Internal/src/ -I op2ext/srcDLL/
netFixClient_LDFLAGS := -shared -LOP2Internal/
netFixClient_LDLIBS := -lOP2Internal -lws2_32

.PHONY: all op2internal op2ext

# By default only compile intermediate files, do not link
all: intermediate-netFixClient

op2internal:
	make -C OP2Internal/

op2ext:
	make -C op2ext/ op2extDll

NetFix.dll: | op2internal op2ext

$(eval $(call DefineCppProject,netFixClient,NetFix.dll,client/))


# Build rules relating to Docker images

DockerFolder := ${TopLevelFolder}/.circleci/
DockerRunFlags := --volume ${TopLevelFolder}:/code
DockerRepository := outpostuniverse

ImageName := netfix-mingw
ImageVersion := 1.0

.PHONY: build-image
build-image:
	docker build ${DockerFolder}/ --file ${DockerFolder}/${ImageName}.Dockerfile --tag ${DockerRepository}/${ImageName}:latest --tag ${DockerRepository}/${ImageName}:${ImageVersion}

.PHONY: run-image
run-image:
	docker run ${DockerRunFlags} --rm --tty ${DockerRepository}/${ImageName}

.PHONY: debug-image
debug-image:
	docker run ${DockerRunFlags} --rm --tty --interactive ${DockerRepository}/${ImageName} bash

.PHONY: root-debug-image
root-debug-image:
	docker run ${DockerRunFlags} --rm --tty --interactive --user=0 ${DockerRepository}/${ImageName} bash

.PHONY: push-image
push-image:
	docker push ${DockerRepository}/${ImageName}
