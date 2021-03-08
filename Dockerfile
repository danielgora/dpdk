##############
# Create base image for building
##############
FROM debian:bullseye-slim as dpdk-base

RUN apt-get update -y && \
    apt-get install --no-install-recommends --yes \
    gcc git make diffutils libnuma-dev xxd \
    && rm -rf /var/lib/apt/lists/*
RUN gcc --version

##############
# Build DPDK
##############
FROM dpdk-base as dpdk-build

ARG dpdk_version=
ARG dpdk_branch=adax_master
ARG ncpus=8
ARG enable_lto=n
ARG makeflags=
ARG machine_type=native

# Uncomment these lines to clone DPDK from somewhere.
#WORKDIR /usr/local/share
#RUN git clone https://github.com/danielgora/dpdk.git -b $dpdk_branch --single-branch
#RUN git clone https://dg@mail1.adax.com:10030:/home/dg/src/dpdk.git -b $dpdk_branch --single-branch
WORKDIR /usr/local/share/dpdk
# Use this line to copy the dpdk directory from the local machine.
COPY . .
RUN make T=x86_64-native-linux-gcc O=x86_64-native-linux-gcc config
WORKDIR /usr/local/share/dpdk/x86_64-native-linux-gcc

RUN make -j $ncpus CONFIG_RTE_MACHINE=$machine_type CONFIG_RTE_ENABLE_LTO=$enable_lto $makeflags
RUN make install DESTDIR=/tmp/dpdk

##############
# Copy to a new base image to keep things small...
##############
FROM dpdk-base

ARG dpdk_version=
ARG dpdk_branch=adax_master
ARG enable_lto=n
ARG machine_type=native

COPY --from=dpdk-build /tmp/dpdk /

LABEL com.adax.dpdk.version="$dpdk_version" \
      com.adax.dpdk.branch="$dpdk_branch" \
      com.adax.dpdk.machine_type="$machine_type" \
      com.adax.dpdk.enable_lto="$enable_lto"
