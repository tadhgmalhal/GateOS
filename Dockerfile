FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TARGET=i686-elf
ENV PREFIX=/opt/cross
ENV PATH=$PREFIX/bin:$PATH
ENV BINUTILS_VERSION=2.42
ENV GCC_VERSION=13.2.0

RUN apt-get update && apt-get install -y \
    build-essential bison flex libgmp-dev \
    libmpc-dev libmpfr-dev texinfo wget \
    nasm grub-pc-bin grub-common xorriso \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

RUN wget -q https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz && \
    tar -xf binutils-$BINUTILS_VERSION.tar.gz && \
    mkdir build-binutils && cd build-binutils && \
    ../binutils-$BINUTILS_VERSION/configure \
      --target=$TARGET --prefix=$PREFIX \
      --disable-nls --disable-werror && \
    make -j$(nproc) && make install && \
    cd /tmp && rm -rf *

RUN wget -q https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz && \
    tar -xf gcc-$GCC_VERSION.tar.gz && \
    mkdir build-gcc && cd build-gcc && \
    ../gcc-$GCC_VERSION/configure \
      --target=$TARGET --prefix=$PREFIX \
      --disable-nls --enable-languages=c,c++ \
      --without-headers && \
    make -j$(nproc) all-gcc && \
    make -j$(nproc) all-target-libgcc && \
    make install-gcc && make install-target-libgcc && \
    cd /tmp && rm -rf *

WORKDIR /gateos
