FROM ubuntu:16.04

RUN apt-get update && \
  apt-get install -y git mercurial python unzip unrar build-essential cmake wget \
  net-tools tcpdump iptables iproute2 iputils-ping vim

RUN wget https://www.nsnam.org/release/ns-allinone-3.29.tar.bz2 && \
  tar xjf ns-allinone-3.29.tar.bz2

WORKDIR /ns-allinone-3.29/ns-3.29

COPY bifrost ./src/bifrost

RUN ./waf configure --enable-sudo --enable-tests --enable-examples
RUN ./waf build

COPY bifrost-run.sh .
RUN chmod +x ./bifrost-run.sh

ENTRYPOINT ./bifrost-run.sh