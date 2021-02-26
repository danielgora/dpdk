FROM centos:8

RUN yum -y install gcc git make diffutils numactl-devel which vim-common

ARG dpdk_branch=adax_master
ARG ncpus=8

#WORKDIR /usr/local/share
#RUN git clone https://github.com/danielgora/dpdk.git -b $dpdk_branch --single-branch
#RUN git clone https://dg@mail1.adax.com:10030:/home/dg/src/dpdk.git -b $dpdk_branch --single-branch
WORKDIR /usr/local/share/dpdk
COPY . .
RUN make T=x86_64-native-linux-gcc O=x86_64-native-linux-gcc config
WORKDIR /usr/local/share/dpdk/x86_64-native-linux-gcc
RUN make -j $ncpus
