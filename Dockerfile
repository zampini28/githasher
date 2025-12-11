FROM debian:trixie AS builder

WORKDIR /app
USER root

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    libarchive-dev \
    libjsoncpp-dev \
    uuid-dev \
    openssl \
    libssl-dev \
    zlib1g-dev \
    libc-ares-dev \
    nlohmann-json3-dev \
    libfmt-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/drogonframework/drogon && \
    cd drogon && \
    git submodule update --init && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install

COPY . .

RUN rm -rf build && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_CXX_STANDARD=20 \
          -DCMAKE_CXX_STANDARD_REQUIRED=ON \
          .. && \
    make -j$(nproc)

FROM debian:trixie-slim

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    libarchive13 \
    libjsoncpp26 \
    libc-ares2 \
    ca-certificates \
    libssl3 \
    zlib1g \
    libfmt10 \
    locales \
    && rm -rf /var/lib/apt/lists/*

RUN echo "en_US.UTF-8 UTF-8" > /etc/locale.gen && \
    locale-gen en_US.UTF-8 && \
    update-locale LANG=en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

RUN mkdir -p logs

COPY --from=builder /app/build/GitHasher /app/GitHasher
COPY config.json /app/config.json

COPY static /app/static

COPY --from=builder /usr/local/lib/libdrogon.so* /usr/lib/
COPY --from=builder /usr/local/lib/libtrantor.so* /usr/lib/

RUN ldconfig

EXPOSE 8080
CMD ["./GitHasher"]
