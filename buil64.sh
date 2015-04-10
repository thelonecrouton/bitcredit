bash autogen.sh

CPPFLAGS="-I/c/deps64/db-4.8.30.NC/build_unix \
-I/c/deps64/openssl-1.0.1l/include \
-I/c/deps64 \
-I/c/deps64/protobuf-2.6.1/src \
-I/c/deps64/libpng-1.6.16 \
-I/c/deps64/qrencode-3.4.4" \
LDFLAGS="-L/c/deps64/db-4.8.30.NC/build_unix \
-L/c/deps64/openssl-1.0.1l \
-L/c/deps64/miniupnpc \
-L/c/deps64/protobuf-2.6.1/src/.libs \
-L/c/deps64/libpng-1.6.16/.libs \
-L/c/deps64/qrencode-3.4.4/.libs" \
BOOST_ROOT=/c/deps64/boost_1_57_0 \
./configure \
--disable-upnp-default \
--disable-tests \
--with-PACKAGE=yes \
--with-qt-incdir=/c/Qt64/5.3.2/include \
--with-qt-libdir=/c/Qt64/5.3.2/lib \
--with-qt-plugindir=/c/Qt64/5.3.2/plugins \
--with-qt-bindir=/c/Qt64/5.3.2/bin \
--with-protoc-bindir=/c/deps64/protobuf-2.6.1/src
