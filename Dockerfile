FROM centos:8

RUN yum -y install gcc git make diffutils numactl-devel which vim-common

ARG dpdk_branch=adax_master
ARG ncpus=8
ARG enable_lto=n

# Uncomment these lines to clone DPDK from somewhere.
#WORKDIR /usr/local/share
#RUN git clone https://github.com/danielgora/dpdk.git -b $dpdk_branch --single-branch
#RUN git clone https://dg@mail1.adax.com:10030:/home/dg/src/dpdk.git -b $dpdk_branch --single-branch
WORKDIR /usr/local/share/dpdk
# Use this line to copy the dpdk directory from the local machine.
COPY . .
RUN make T=x86_64-native-linux-gcc O=x86_64-native-linux-gcc config
WORKDIR /usr/local/share/dpdk/x86_64-native-linux-gcc
# Set RTE_ENABLE_LTO if requested..
RUN if [ "$enable_lto" = "y" ]; then sed -i "s/RTE_ENABLE_LTO=.*/RTE_ENABLE_LTO=${enable_lto}/" .config; fi
RUN make -j $ncpus
