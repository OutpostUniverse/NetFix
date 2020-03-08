# See Docker section of makefile in root project folder for usage commands.

FROM ubuntu:18.04

# Install base development tools
# Includes tools to build download, unpack, and build source packages
# Includes tools needed for primary CircleCI containers
RUN apt-get update && apt-get install -y --no-install-recommends \
    mingw-w64=5.0.3-1 \
    cmake=3.10.2-* \
    make=4.1-* \
    binutils=2.30-* \
    git=1:2.17.1-* \
    ssh=1:7.6p1-* \
    curl=7.58.0-* \
    tar=1.29b-* \
    gzip=1.6-* \
    bzip2=1.0.6-* \
    gnupg=2.2.4-* \
    software-properties-common=0.96.24.32.12 \
    ca-certificates=* \
  && rm -rf /var/lib/apt/lists/*

# Set custom variables for build script convenience
# Set default target OS
ENV TARGET_OS=Windows
# Set architecture short names
ENV ARCH64=x86_64-w64-mingw32
ENV ARCH32=i686-w64-mingw32
# Set compiler short names
ENV CXX64=${ARCH64}-g++
ENV  CC64=${ARCH64}-gcc
ENV  LD64=${ARCH64}-ld
ENV CXX32=${ARCH32}-g++
ENV  CC32=${ARCH32}-gcc
ENV  LD32=${ARCH32}-ld

# Install wine so resulting unit test binaries can be run
RUN curl -L https://dl.winehq.org/wine-builds/winehq.key | apt-key add - && \
  apt-add-repository 'deb https://dl.winehq.org/wine-builds/ubuntu/ bionic main' && \
  dpkg --add-architecture i386 && \
  apt-get update && apt-get install -y --no-install-recommends \
    wine-stable-amd64=4.0.3~bionic \
    wine-stable-i386=4.0.3~bionic \
    wine-stable=4.0.3~bionic \
    winehq-stable=4.0.3~bionic \
  && rm -rf /var/lib/apt/lists/*

# Set default install location for custom packages
ENV INSTALL_PREFIX=/usr/local/
ENV INSTALL64=${INSTALL_PREFIX}${ARCH64}/
ENV INSTALL32=${INSTALL_PREFIX}${ARCH32}/

# Create directories for local install of libraries
RUN mkdir --parents "${INSTALL64}" "${INSTALL32}"

# Download, compile, and install Google Test source package
WORKDIR /tmp/gtest/
RUN curl --location https://github.com/google/googletest/archive/release-1.10.0.tar.gz | tar -xz && \
  cmake -Hgoogletest-release-1.10.0/ -B"${ARCH64}" -DCMAKE_CXX_COMPILER="${CXX64}" -DCMAKE_C_COMPILER="${CC64}" -DCMAKE_CXX_FLAGS="-std=c++17" -DCMAKE_SYSTEM_NAME="${TARGET_OS}" -Dgtest_disable_pthreads=ON && make -C "${ARCH64}" && \
  cmake -Hgoogletest-release-1.10.0/ -B"${ARCH64}" -DCMAKE_CXX_COMPILER="${CXX64}" -DCMAKE_C_COMPILER="${CC64}" -DCMAKE_CXX_FLAGS="-std=c++17" -DCMAKE_SYSTEM_NAME="${TARGET_OS}" -Dgtest_disable_pthreads=ON -DBUILD_SHARED_LIBS=ON && make -C "${ARCH64}" && \
  cmake -Hgoogletest-release-1.10.0/ -B"${ARCH32}" -DCMAKE_CXX_COMPILER="${CXX32}" -DCMAKE_C_COMPILER="${CC32}" -DCMAKE_CXX_FLAGS="-std=c++17" -DCMAKE_SYSTEM_NAME="${TARGET_OS}" -Dgtest_disable_pthreads=ON && make -C "${ARCH32}" && \
  cmake -Hgoogletest-release-1.10.0/ -B"${ARCH32}" -DCMAKE_CXX_COMPILER="${CXX32}" -DCMAKE_C_COMPILER="${CC32}" -DCMAKE_CXX_FLAGS="-std=c++17" -DCMAKE_SYSTEM_NAME="${TARGET_OS}" -Dgtest_disable_pthreads=ON -DBUILD_SHARED_LIBS=ON && make -C "${ARCH32}" && \
  cp --parents -r \
    "${ARCH64}/bin/" \
    "${ARCH64}/lib/" \
    "${ARCH32}/bin/" \
    "${ARCH32}/lib/" \
    "${INSTALL_PREFIX}" && \
  mkdir -p \
    "${INSTALL_PREFIX}share/mingw-w64/" \
    "${INSTALL64}include/" \
    "${INSTALL32}include/" && \
  cp -r \
    googletest-release-1.10.0/googletest/include/ \
    googletest-release-1.10.0/googlemock/include/ \
    "${INSTALL_PREFIX}share/mingw-w64/" && \
  ln -sf "${INSTALL_PREFIX}share/mingw-w64/include/gtest/" "${INSTALL64}include/" && \
  ln -sf "${INSTALL_PREFIX}share/mingw-w64/include/gmock/" "${INSTALL64}include/" && \
  ln -sf "${INSTALL_PREFIX}share/mingw-w64/include/gtest/" "${INSTALL32}include/" && \
  ln -sf "${INSTALL_PREFIX}share/mingw-w64/include/gmock/" "${INSTALL32}include/" && \
  cp --parents -r \
    googletest-release-1.10.0/CMakeLists.txt \
    googletest-release-1.10.0/googletest/ \
    googletest-release-1.10.0/googlemock/ \
    "${INSTALL_PREFIX}src/" && \
  rm -rf /tmp/gtest/

# Custom variables for install locations
ENV INCLUDE64=${INSTALL64}include/
ENV INCLUDE32=${INSTALL32}include/
ENV LIB64=${INSTALL64}lib;/usr/${ARCH64}/lib
ENV LIB32=${INSTALL32}lib;/usr/${ARCH32}/lib
ENV BIN64=${INSTALL64}bin/
ENV BIN32=${INSTALL32}bin/
ENV PATH64="${PATH}:${BIN64}"
ENV PATH32="${PATH}:${BIN32}"
ENV WINEPATH64=${BIN64};/usr/lib/gcc/${ARCH64}/7.3-win32/
ENV WINEPATH32=${BIN32};/usr/lib/gcc/${ARCH32}/7.3-win32/

# Setup compiler and tooling default folders
ENV CPLUS_INCLUDE_PATH="${INCLUDE32}"
ENV LIBRARY_PATH="${LIB32}"
ENV PATH="${PATH32}"
ENV WINEPATH="${WINEPATH32}"

# Set default compiler
ENV CXX=${CXX32}
ENV  CC=${CC32}

RUN useradd -m -s /bin/bash user
USER user

VOLUME /code
WORKDIR /code

# Pre-setup Wine to save startup time later
RUN wineboot

CMD ["make", "--keep-going"]
