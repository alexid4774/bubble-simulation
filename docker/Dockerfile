FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        wget \
        bash \
        libopenmpi-dev \
        openmpi-bin && \
    rm -rf /var/lib/apt/lists/*

RUN wget -O /tmp/openfoam.deb \
    https://dl.openfoam.org/ubuntu/dists/jammy/main/binary-arm64/openfoam14_20261008_arm64.deb && \
    apt-get update && \
    apt-get install -y /tmp/openfoam.deb && \
    rm -f /tmp/openfoam.deb && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]
