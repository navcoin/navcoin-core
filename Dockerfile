FROM ubuntu:16.04

RUN apt-get update --fix-missing
RUN apt-get install -y build-essential libcurl3-dev libunbound-dev libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils
RUN apt-get install -y libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev libboost-all-dev
RUN apt-get install -y libcurl4-openssl-dev vim curl
RUN apt-get install -y libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler
RUN apt-get install -y libzmq3-dev

RUN apt-get -y install software-properties-common
RUN add-apt-repository ppa:bitcoin/bitcoin
RUN apt-get -y update
RUN apt-get -y install libdb4.8-dev libdb4.8++-dev git

RUN git config --global user.email "you@example.com"
RUN git config --global user.name "Your Name"

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y git build-essential libtool autoconf automake pkg-config unzip libkrb5-dev
RUN cd /tmp && git clone https://github.com/jedisct1/libsodium.git && cd libsodium && git checkout e2a30a && ./autogen.sh && ./configure && make check && make install && ldconfig
RUN cd /tmp && git clone --depth 1 https://github.com/zeromq/libzmq.git && cd libzmq && ./autogen.sh && ./configure && make
# RUN cd /tmp/libzmq && make check
RUN cd /tmp/libzmq && make install && ldconfig

RUN apt-get -y install libexpat-dev
RUN apt-get -y install libexpat1
RUN apt-get -y install libunbound-dev
RUN cd /tmp && git clone https://github.com/NLnetLabs/unbound.git && cd unbound && ./configure && make && make install

ENTRYPOINT ["/data/docker/entrypoint.sh"]