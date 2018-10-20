FROM debian:jessie-20181011

RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    gcc \
    libwaffle-dev \
    ninja-build \
    python3-pip

RUN pip3 install attrs

ADD container.py /

ENTRYPOINT /container.py
