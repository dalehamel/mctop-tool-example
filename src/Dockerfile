FROM alpine:3.10 as builder

# ensure SASL's "libplain.so" is installed as per https://github.com/memcached/memcached/wiki/SASLHowto
RUN apk add --no-cache cyrus-sasl-plain wget binutils

ENV MEMCACHED_VERSION 1.5.19
ENV MEMCACHED_SHA1 14e6a02e743838696fcb620edf6a2fd7e60cabec

# Get dtrace dependencies for alpine in a kinda hacky way
RUN mkdir /build && cd /build && wget http://launchpadlibrarian.net/251391227/systemtap-sdt-dev_2.9-2ubuntu2_amd64.deb && \
    ar x systemtap-sdt-dev_2.9-2ubuntu2_amd64.deb && \
    tar -xpf data.tar.xz && mv usr/bin/dtrace /usr/bin/dtrace && \
    mkdir -p /usr/include/sys && \
    mv usr/include/x86_64-linux-gnu/sys/* /usr/include/sys && rm -rf /build

RUN set -x \
        \
        && apk add --no-cache --virtual .build-deps \
                ca-certificates \
                coreutils \
                cyrus-sasl-dev \
                dpkg-dev dpkg \
                gcc \
                libc-dev \
                libevent-dev \
                linux-headers \
                make \
                openssl \
                perl \
                perl-utils \
                tar \
                wget \
        \
        && wget -O memcached.tar.gz "https://memcached.org/files/memcached-$MEMCACHED_VERSION.tar.gz" \
        && echo "$MEMCACHED_SHA1  memcached.tar.gz" | sha1sum -c - \
        && mkdir -p /usr/src/memcached \
        && tar -xzf memcached.tar.gz -C /usr/src/memcached --strip-components=1 \
        && rm memcached.tar.gz \
        \
        && cd /usr/src/memcached \
        \
        && gnuArch="$(dpkg-architecture --query DEB_BUILD_GNU_TYPE)" \
        && enableExtstore="$( \
# https://github.com/docker-library/memcached/pull/38
                case "$gnuArch" in \
# https://github.com/memcached/memcached/issues/381 "--enable-extstore on s390x (IBM System Z mainframe architecture) fails tests"
                        s390x-*) ;; \
                        *) echo '--enable-extstore' ;; \
                esac \
        )" \
        && ./configure \
        CFLAGS="-ggdb3" \
        CXXFLAGS="-ggdb3" \
        LDFLAGS="-ggdb3" \
                --build="$gnuArch" \
                --enable-sasl \
                --enable-sasl-pwdb \
                --enable-dtrace \
                $enableExtstore \
        && make -j "$(nproc)" \
        \
    && make test \
        && make install \
        && memcached -V

FROM alpine:3.10

ENV UID 11211
ENV GID 11211

RUN apk update && apk upgrade && apk add --no-cache coreutils \
    libevent \
    libsasl \
    file \
    bash && \
# add our user and group first to make sure their IDs get assigned consistently, regardless of whatever dependencies get added
    addgroup -g $GID memcache && adduser -D -u $UID -G memcache memcache
COPY --from=builder /usr/local/bin/memcached /usr/local/bin/memcached

ENTRYPOINT ["memcached"]
CMD ["memcached"]
USER $UID:$GID
