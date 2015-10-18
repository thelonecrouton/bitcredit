TEMPLATE = app
TARGET = bitcredit-qt
VERSION = 0.30.17.8
INCLUDEPATH += src src/json src/qt src/qt/forms src/compat src/crypto src/lz4 src/primitives src/script src/secp256k1/include src/univalue src/xxhash
DEFINES += ENABLE_WALLET HAVE_WORKING_BOOST_SLEEP
DEFINES += QT_GUI BOOST_THREAD_USE_LIB BOOST_SPIRIT_THREADSAFE CURL_STATICLIB
CONFIG += no_include_pwd
CONFIG += thread static
QT += core gui network printsupport widgets

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
}

# Use command line:
#   qmake xxx.pro RELEASE=1 USE_UPNP=1 -config release QMAKE_LFLAGS+="-static-libgcc -static-libstdc++" BOOST_INCLUDE_PATH=C:/MinGW/msys/1.0/local/include BOOST_LIB_PATH=C:/MinGW/msys/1.0/local/lib BOOST_LIB_SUFFIX=-mgw46-mt-s-1_54
#   make -f Makefile.Release
#

SECP256K1_INCLUDE_PATH = $$PWD/src/secp256k1/include
SECP256K1_LIB_PATH = $$PWD/src/secp256k1/.libs

OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build

USE_UPNP=1
USE_QRCODE=1

# use: qmake "RELEASE=1"
contains(RELEASE, 1) {
        # Mac: compile for maximum compatibility (10.5, 32-bit)
        macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.5 -arch x86_64 -isysroot /Developer/SDKs/MacOSX10.5.sdk

    !windows:!macx {
        # Linux: static link
        LIBS += -Wl,-Bstatic -Wl,-z,relro -Wl,-z,now
    }
}

!win32 {
# for extra security against potential buffer overflows: enable GCCs Stack Smashing Protection
QMAKE_CXXFLAGS *= -fstack-protector-all --param ssp-buffer-size=1
QMAKE_LFLAGS *= -fstack-protector-all --param ssp-buffer-size=1
# We need to exclude this for Windows cross compile with MinGW 4.2.x, as it will result in a non-working executable!
# This can be enabled for Windows, when we switch to MinGW >= 4.4.x.
}
# for extra security (see: https://wiki.debian.org/Hardening): this flag is GCC compiler-specific
QMAKE_CXXFLAGS *= -D_FORTIFY_SOURCE=2
# for extra security on Windows: enable ASLR and DEP via GCC linker flags
win32:QMAKE_LFLAGS *= -Wl,--dynamicbase -Wl,--nxcompat -static
win32:QMAKE_LFLAGS += -static-libgcc -static-libstdc++

# use: qmake "USE_QRCODE=1"
# libqrencode (http://fukuchi.org/works/qrencode/index.en.html) must be installed for support
contains(USE_QRCODE, 1) {
    message(Building with QRCode support)
    DEFINES += USE_QRCODE
    LIBS += -lqrencode
}

# use: qmake "USE_UPNP=1" ( enabled by default; default)
#  or: qmake "USE_UPNP=0" (disabled by default)
#  or: qmake "USE_UPNP=-" (not supported)
# miniupnpc (http://miniupnp.free.fr/files/) must be installed for support
contains(USE_UPNP, -) {
    message(Building without UPNP support)
} else {
    message(Building with UPNP support)
    count(USE_UPNP, 0) {
        USE_UPNP=1
    }
    DEFINES += USE_UPNP=$$USE_UPNP MINIUPNP_STATICLIB
    INCLUDEPATH += $$MINIUPNPC_INCLUDE_PATH
    LIBS += $$join(MINIUPNPC_LIB_PATH,,-L,) -lminiupnpc
    win32:LIBS += -liphlpapi
}

#use: qmake "USE_DBUS=1" or qmake "USE_DBUS=0"
linux:count(USE_DBUS, 0) {
    USE_DBUS=1
}
contains(USE_DBUS, 1) {
   message(Building with DBUS (Freedesktop notifications) support)
    DEFINES += USE_DBUS
    QT += dbus
}

# use: qmake "USE_IPV6=1" ( enabled by default; default)
#  or: qmake "USE_IPV6=0" (disabled by default)
#  or: qmake "USE_IPV6=-" (not supported)
contains(USE_IPV6, -) {
    message(Building without IPv6 support)
} else {
    message(Building with IPv6 support)
    count(USE_IPV6, 0) {
        USE_IPV6=1
    }
    DEFINES += USE_IPV6=$$USE_IPV6
}

include (protobuf.pri)
PROTOS += src/qt/paymentrequest.proto \

contains(BITCOIN_NEED_QT_PLUGINS, 1) {
    DEFINES += BITCOIN_NEED_QT_PLUGINS
    QTPLUGIN += qcncodecs qjpcodecs qtwcodecs qkrcodecs qtaccessiblewidgets
}

# LIBSEC256K1 SUPPORT
QMAKE_CXXFLAGS *= -DUSE_SECP256K1

INCLUDEPATH += src/leveldb/include src/leveldb/helpers/memenv
LIBS += $$PWD/src/leveldb/libleveldb.a $$PWD/src/leveldb/libmemenv.a
!win32 {
        # we use QMAKE_CXXFLAGS_RELEASE even without RELEASE=1 because we use RELEASE to indicate linking preferences not -O preferences
        genleveldb.commands = cd $$PWD/src/leveldb && CC=$$QMAKE_CC CXX=$$QMAKE_CXX $(MAKE) OPT=\"$$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE\" libleveldb.a libmemenv.a
} else {
        # make an educated guess about what the ranlib command is called
        isEmpty(QMAKE_RANLIB) {
                QMAKE_RANLIB = $$replace(QMAKE_STRIP, strip, ranlib)
        }
        LIBS += -lshlwapi
       # genleveldb.commands = cd $$PWD/src/leveldb && CC=$$QMAKE_CC CXX=$$QMAKE_CXX TARGET_OS=OS_WINDOWS_CROSSCOMPILE $(MAKE) OPT=\"$$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE\" libleveldb.a libmemenv.a && $$QMAKE_RANLIB $$PWD/src/leveldb/libleveldb.a && $$QMAKE_RANLIB $$PWD/src/leveldb/libmemenv.a
}
genleveldb.target = src/leveldb/libleveldb.a
genleveldb.depends = FORCE
PRE_TARGETDEPS += src/leveldb/libleveldb.a
QMAKE_EXTRA_TARGETS += genleveldb
# Gross ugly hack that depends on qmake internals, unfortunately there is no other way to do it.
QMAKE_CLEAN += src/leveldb/libleveldb.a; cd $$PWD/src/leveldb ; $(MAKE) clean

# regenerate src/build.h
!windows|contains(USE_BUILD_INFO, 1) {
    genbuild.depends = FORCE
    genbuild.commands = cd $$PWD; /bin/sh share/genbuild.sh $$OUT_PWD/build/build.h
    genbuild.target = $$OUT_PWD/build/build.h
    PRE_TARGETDEPS += $$OUT_PWD/build/build.h
    QMAKE_EXTRA_TARGETS += genbuild
    DEFINES += HAVE_BUILD_INFO
}

contains(USE_O3, 1) {
        message(Building O3 optimization flag)
        QMAKE_CXXFLAGS_RELEASE -= -O2
        QMAKE_CFLAGS_RELEASE -= -O2
        QMAKE_CXXFLAGS += -O3
        QMAKE_CFLAGS += -O3
}

*-g++-32 {
    message("32 platform, adding -msse2 flag")

    QMAKE_CXXFLAGS += -msse2
    QMAKE_CFLAGS += -msse2
}

QMAKE_CXXFLAGS_WARN_ON = -fdiagnostics-show-option -Wall -Wextra -Wno-ignored-qualifiers -Wformat -Wformat-security -Wno-unused-parameter -Wstack-protector


# Input
DEPENDPATH += json qt
HEADERS += src/qt/bitcreditgui.h \
  src/activebanknode.h \
  src/addrman.h \
  src/alert.h \
  src/allocators.h \
  src/amount.h \
  src/base58.h \
  src/bidtracker.h \
  src/bloom.h \
  src/chain.h \
  src/chainparams.h \
  src/chainparamsbase.h \
  src/chainparamsseeds.h \
  src/checkpoints.h \
  src/checkqueue.h \
  src/clientversion.h \
  src/coincontrol.h \
  src/coins.h \
  src/compat.h \
  src/compressor.h \
  src/primitives/block.h \
  src/primitives/transaction.h \
  src/core_io.h \
  src/crypter.h \
  src/darksend.h \
  src/darksend-relay.h \
  src/db.h \
  src/eccryptoverify.h \
  src/ecwrapper.h \
  src/hash.h \
  src/init.h \
  src/instantx.h \
  src/key.h \
  src/keepass.h \
  src/keystore.h \
  src/leveldbwrapper.h \
  src/limitedmap.h \
  src/lz4/lz4.h \
  src/main.h \
  src/banknode.h \
  src/banknode-pos.h \
  src/banknodeman.h \
  src/banknodeconfig.h \
  src/merkleblock.h \
  src/miner.h \
  src/momentum.h \
  src/mruset.h \
  src/netbase.h \
  src/net.h \
  src/noui.h \
  src/pow.h \
  src/protocol.h \
  src/pubkey.h \
  src/random.h \
  src/rpcclient.h \
  src/rpcprotocol.h \
  src/rpcserver.h \
  src/script/interpreter.h \
  src/script/script.h \
  src/script/sigcache.h \
  src/script/sign.h \
  src/script/standard.h \
  src/script/script_error.h \
  src/semiOrderedMap.h \
  src/serialize.h \
  src/smessage.h \
  src/streams.h \
  src/spork.h \
  src/sync.h \
  src/threadsafety.h \
  src/timedata.h \
  src/tinyformat.h \
  src/txdb.h \
  src/txmempool.h \
  src/ui_interface.h \
  src/uint256.h \
  src/undo.h \
  src/util.h \
  src/utilstrencodings.h \
  src/utilmoneystr.h \
  src/utiltime.h \
  src/version.h \
  src/wallet.h \
  src/wallet_ismine.h \
  src/walletdb.h \
  src/strlcpy.h \
  src/compat/sanity.h \
  src/xxhash/xxhash.h \
  src/rawdata.h \
  src/bankmath.h \
  src/ibtp.h \
  src/voting.h \
  src/json/json_spirit.h \
  src/json/json_spirit_error_position.h \
  src/json/json_spirit_reader.h \
  src/json/json_spirit_reader_template.h \
  src/json/json_spirit_stream_reader.h \
  src/json/json_spirit_utils.h \
  src/json/json_spirit_value.h \
  src/json/json_spirit_writer.h \
  src/json/json_spirit_writer_template.h \
  src/crypto/common.h \
  src/crypto/sha256.h \
  src/crypto/sha512.h \
  src/crypto/hmac_sha256.h \
  src/crypto/rfc6979_hmac_sha256.h \
  src/crypto/hmac_sha512.h \
  src/crypto/sha1.h \
  src/crypto/ripemd160.h \
  src/univalue/univalue_escapes.h \
  src/univalue/univalue.h \
  src/qt/addressbookpage.h \
  src/qt/addresstablemodel.h \
  src/qt/askpassphrasedialog.h \
  src/qt/bitcreditaddressvalidator.h \
  src/qt/bitcreditamountfield.h \
  src/qt/bitcreditunits.h \
  src/qt/clientmodel.h \
  src/qt/coincontroldialog.h \
  src/qt/coincontroltreewidget.h \
  src/qt/csvmodelwriter.h \
  src/qt/darksendconfig.h \
  src/qt/editaddressdialog.h \
  src/qt/guiconstants.h \
  src/qt/guiutil.h \
  src/qt/intro.h \
  src/qt/macnotificationhandler.h \
  src/qt/networkstyle.h \
  src/qt/ircmodel.h \
  src/qt/notificator.h \
  src/qt/openuridialog.h \
  src/qt/optionsdialog.h \
  src/qt/optionsmodel.h \
  src/qt/overviewpage.h \
  src/qt/paymentrequestplus.h \
  src/qt/paymentserver.h \
  src/qt/peertablemodel.h \
  src/qt/qvalidatedlineedit.h \
  src/qt/qvalidatedtextedit.h \
  src/qt/qvaluecombobox.h \
  src/qt/receivecoinsdialog.h \
  src/qt/receiverequestdialog.h \
  src/qt/recentrequeststablemodel.h \
  src/qt/rpcconsole.h \
  src/qt/sendcoinsdialog.h \
  src/qt/sendcoinsentry.h \
  src/qt/signverifymessagedialog.h \
  src/qt/splashscreen.h \
  src/qt/trafficgraphwidget.h \
  src/qt/transactiondesc.h \
  src/qt/transactiondescdialog.h \
  src/qt/transactionfilterproxy.h \
  src/qt/transactionrecord.h \
  src/qt/transactiontablemodel.h \
  src/qt/transactionview.h \
  src/qt/utilitydialog.h \
  src/qt/walletframe.h \
  src/qt/walletmodel.h \
  src/qt/walletmodeltransaction.h \
  src/qt/walletview.h \
  src/qt/winshutdownmonitor.h \
  src/qt/blockbrowser.h \
  src/qt/bankstatisticspage.h \
  src/qt/verticallabel.h \
  src/qt/qcustomplot.h \
  src/qt/exchangebrowser.h \
  src/qt/chatwindow.h \
  src/qt/serveur.h \
  src/qt/sendmessagesentry.h \
  src/qt/sendmessagesdialog.h \
  src/qt/receiptpage.h \
  src/qt/messagepage.h \
  src/qt/invoicepage.h \
  src/qt/invoiceviewpage.h \
  src/qt/messagemodel.h \
  src/qt/banknodemanager.h \
  src/qt/adrenalinenodeconfigdialog.h \
  src/qt/bidpage.h \
  src/qt/addeditadrenalinenode.h \
  src/qt/paymentrequest.pb.h \
  src/qt/vanity_avl.h \
  src/qt/vanity_util.h \
  src/qt/vanity_pattern.h \
  src/qt/vanitygenpage.h \
  src/qt/vanitygenwork.h \
  src/qt/miningpage.h \
  src/qt/mininginfopage.h \
  src/qt/blockexplorer.h \
  src/qt/votecoinsdialog.h \
  src/qt/votecoinsentry.h \
  src/qt/carousel.h \
  src/qt/pixmap.h

SOURCES += src/qt/bitcredit.cpp src/qt/bitcreditgui.cpp \
  src/qt/bitcreditaddressvalidator.cpp \
  src/qt/bitcreditamountfield.cpp \
  src/qt/bitcreditunits.cpp \
  src/qt/clientmodel.cpp \
  src/qt/csvmodelwriter.cpp \
  src/qt/darksendconfig.cpp \
  src/qt/guiutil.cpp \
  src/qt/intro.cpp \
  src/qt/networkstyle.cpp \
  src/qt/ircmodel.cpp \
  src/qt/notificator.cpp \
  src/qt/optionsdialog.cpp \
  src/qt/optionsmodel.cpp \
  src/qt/peertablemodel.cpp \
  src/qt/qvalidatedlineedit.cpp \
  src/qt/qvaluecombobox.cpp \
  src/qt/rpcconsole.cpp \
  src/qt/splashscreen.cpp \
  src/qt/trafficgraphwidget.cpp \
  src/qt/utilitydialog.cpp \
  src/qt/winshutdownmonitor.cpp \
  src/qt/blockbrowser.cpp \
  src/qt/bankstatisticspage.cpp \
  src/qt/qcustomplot.cpp \
  src/qt/exchangebrowser.cpp \
  src/qt/chatwindow.cpp \
  src/qt/serveur.cpp \
  src/qt/qvalidatedtextedit.cpp \
  src/qt/messagemodel.cpp \
  src/qt/addressbookpage.cpp \
  src/qt/addresstablemodel.cpp \
  src/qt/adrenalinenodeconfigdialog.cpp \
  src/qt/addeditadrenalinenode.cpp \
  src/qt/askpassphrasedialog.cpp \
  src/qt/banknodemanager.cpp \
  src/qt/coincontroldialog.cpp \
  src/qt/coincontroltreewidget.cpp \
  src/qt/editaddressdialog.cpp \
  src/qt/openuridialog.cpp \
  src/qt/overviewpage.cpp \
  src/qt/paymentrequestplus.cpp \
  src/qt/paymentserver.cpp \
  src/qt/receivecoinsdialog.cpp \
  src/qt/receiverequestdialog.cpp \
  src/qt/recentrequeststablemodel.cpp \
  src/qt/sendcoinsdialog.cpp \
  src/qt/sendcoinsentry.cpp \
  src/qt/signverifymessagedialog.cpp \
  src/qt/transactiondesc.cpp \
  src/qt/transactiondescdialog.cpp \
  src/qt/transactionfilterproxy.cpp \
  src/qt/transactionrecord.cpp \
  src/qt/transactiontablemodel.cpp \
  src/qt/transactionview.cpp \
  src/qt/walletframe.cpp \
  src/qt/walletmodel.cpp \
  src/qt/walletmodeltransaction.cpp \
  src/qt/walletview.cpp \
  src/qt/verticallabel.cpp \
  src/qt/sendmessagesentry.cpp \
  src/qt/sendmessagesdialog.cpp \
  src/qt/receiptpage.cpp \
  src/qt/messagepage.cpp \
  src/qt/invoicepage.cpp \
  src/qt/invoiceviewpage.cpp \
  src/qt/bidpage.cpp \
  src/qt/votecoinsdialog.cpp \
  src/qt/votecoinsentry.cpp \
  src/activebanknode.cpp \
  src/rpcclient.cpp \
  src/addrman.cpp \
  src/alert.cpp \
  src/bloom.cpp \
  src/chain.cpp \
  src/checkpoints.cpp \
  src/init.cpp \
  src/leveldbwrapper.cpp \
  src/lz4/lz4.c \
  src/main.cpp \
  src/merkleblock.cpp \
  src/miner.cpp \
  src/net.cpp \
  src/noui.cpp \
  src/pow.cpp \
  src/rest.cpp \
  src/rpcblockchain.cpp \
  src/rpcdarksend.cpp \
  src/rpcsmessage.cpp \
  src/rpcmining.cpp \
  src/rpcmisc.cpp \
  src/rpcnet.cpp \
  src/ibtp.cpp \
  src/rpcrawtransaction.cpp \
  src/rpcserver.cpp \
  src/script/sigcache.cpp \
  src/smessage.cpp \
  src/timedata.cpp \
  src/txdb.cpp \
  src/txmempool.cpp \
  src/xxhash/xxhash.c \
  src/json/json_spirit_value.cpp \
  src/json/json_spirit_reader.cpp \
  src/json/json_spirit_writer.cpp \
  src/db.cpp \
  src/crypter.cpp \
  src/rpcdump.cpp \
  src/rpcwallet.cpp \
  src/wallet.cpp \
  src/wallet_ismine.cpp \
  src/walletdb.cpp \
  src/keepass.cpp \
  src/crypto/sha1.cpp \
  src/crypto/sha256.cpp \
  src/crypto/sha512.cpp \
  src/crypto/hmac_sha256.cpp \
  src/crypto/rfc6979_hmac_sha256.cpp \
  src/crypto/hmac_sha512.cpp \
  src/crypto/ripemd160.cpp \
  src/univalue/univalue.cpp \
  src/univalue/univalue_read.cpp \
  src/univalue/univalue_write.cpp \
  src/allocators.cpp \
  src/amount.cpp \
  src/base58.cpp \
  src/rawdata.cpp \
  src/bankmath.cpp \
  src/bidtracker.cpp \
  src/chainparams.cpp \
  src/coins.cpp \
  src/compressor.cpp \
  src/darksend.cpp \
  src/darksend-relay.cpp \
  src/banknode.cpp \
  src/banknode-pos.cpp \
  src/banknodeconfig.cpp \
  src/banknodeman.cpp \
  src/instantx.cpp \
  src/momentum.cpp \
  src/primitives/block.cpp \
  src/primitives/transaction.cpp \
  src/core_read.cpp \
  src/core_write.cpp \
  src/eccryptoverify.cpp \
  src/ecwrapper.cpp \
  src/hash.cpp \
  src/key.cpp \
  src/keystore.cpp \
  src/netbase.cpp \
  src/protocol.cpp \
  src/pubkey.cpp \
  src/script/interpreter.cpp \
  src/script/script.cpp \
  src/script/sign.cpp \
  src/script/standard.cpp \
  src/script/script_error.cpp \
  src/compat/strnlen.cpp \
  src/compat/glibc_sanity.cpp \
  src/compat/glibcxx_sanity.cpp \
  src/chainparamsbase.cpp \
  src/clientversion.cpp \
  src/random.cpp \
  src/rpcprotocol.cpp \
  src/spork.cpp \
  src/sync.cpp \
  src/uint256.cpp \
  src/util.cpp \
  src/utilstrencodings.cpp \
  src/utilmoneystr.cpp \
  src/utiltime.cpp \
  src/qt/vanity_util.cpp \
  src/qt/vanity_pattern.cpp \
  src/qt/vanitygenpage.cpp \
  src/qt/vanitygenwork.cpp \
  src/qt/miningpage.cpp \
  src/qt/mininginfopage.cpp \
  src/qt/blockexplorer.cpp \
  src/qt/carousel.cpp \
  src/qt/pixmap.cpp \
  src/voting.cpp \

RESOURCES += \
    src/qt/bitcredit.qrc\
    src/qt/bitcredit_locale.qrc

FORMS += \
  src/qt/forms/addressbookpage.ui \
  src/qt/forms/askpassphrasedialog.ui \
  src/qt/forms/coincontroldialog.ui \
  src/qt/forms/darksendconfig.ui \
  src/qt/forms/editaddressdialog.ui \
  src/qt/forms/intro.ui \
  src/qt/forms/openuridialog.ui \
  src/qt/forms/optionsdialog.ui \
  src/qt/forms/overviewpage.ui \
  src/qt/forms/receivecoinsdialog.ui \
  src/qt/forms/receiverequestdialog.ui \
  src/qt/forms/rpcconsole.ui \
  src/qt/forms/sendcoinsdialog.ui \
  src/qt/forms/sendcoinsentry.ui \
  src/qt/forms/signverifymessagedialog.ui \
  src/qt/forms/transactiondescdialog.ui \
  src/qt/forms/blockbrowser.ui \
  src/qt/forms/bankstatisticspage.ui \
  src/qt/forms/paperwalletdialog.ui \
  src/qt/forms/exchangebrowser.ui \
  src/qt/forms/chatwindow.ui \
  src/qt/forms/sendmessagesentry.ui \
  src/qt/forms/sendmessagesdialog.ui \
  src/qt/forms/receiptpage.ui \
  src/qt/forms/messagepage.ui \
  src/qt/forms/invoicepage.ui \
  src/qt/forms/invoiceviewpage.ui \
  src/qt/forms/addeditadrenalinenode.ui \
  src/qt/forms/adrenalinenodeconfigdialog.ui \
  src/qt/forms/bidpage.ui \
  src/qt/forms/banknodemanager.ui \
  src/qt/forms/helpmessagedialog.ui \
  src/qt/forms/miningpage.ui \
  src/qt/forms/mininginfopage.ui \
  src/qt/forms/vanitygenpage.ui \
  src/qt/forms/blockexplorer.ui \
  src/qt/forms/votecoinsentry.ui \
  src/qt/forms/votecoinsdialog.ui \

CODECFORTR = UTF-8

# for lrelease/lupdate
# also add new translations to src/qt/bitcoin.qrc under translations/
TRANSLATIONS = $$files(src/qt/locale/bitcredit_*.ts)

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
isEmpty(QM_DIR):QM_DIR = $$PWD/src/qt/locale
# automatically build translations, so they can be included in resource file
TSQM.name = lrelease ${QMAKE_FILE_IN}
TSQM.input = TRANSLATIONS
TSQM.output = $$QM_DIR/${QMAKE_FILE_BASE}.qm
TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
TSQM.CONFIG = no_link
QMAKE_EXTRA_COMPILERS += TSQM

# "Other files" to show in Qt Creator
OTHER_FILES += \
        doc/*.rst doc/*.txt doc/README README.md res/bitcoin-qt.rc

# platform specific defaults, if not overridden on command line
isEmpty(BOOST_LIB_SUFFIX) {
    macx:BOOST_LIB_SUFFIX = -mt
    windows:BOOST_LIB_SUFFIX = -mgw49-mt-s-1_54
}

isEmpty(BOOST_THREAD_LIB_SUFFIX) {
    BOOST_THREAD_LIB_SUFFIX = $$BOOST_LIB_SUFFIX
}

isEmpty(BDB_LIB_PATH) {
    macx:BDB_LIB_PATH = /opt/local/lib/db48
}

isEmpty(BDB_LIB_SUFFIX) {
    macx:BDB_LIB_SUFFIX = -4.8
}

isEmpty(BDB_INCLUDE_PATH) {
    macx:BDB_INCLUDE_PATH = /opt/local/include/db48
}

isEmpty(BOOST_LIB_PATH) {
    macx:BOOST_LIB_PATH = /opt/local/lib
}

isEmpty(BOOST_INCLUDE_PATH) {
    macx:BOOST_INCLUDE_PATH = /opt/local/include
}

windows:DEFINES += WIN32 WIN32_LEAN_AND_MEAN
windows:RC_FILE = src/qt/res/bitcredit-qt-res.rc

windows:!contains(MINGW_THREAD_BUGFIX, 0) {
    # At least qmake's win32-g++-cross profile is missing the -lmingwthrd
    # thread-safety flag. GCC has -mthreads to enable this, but it doesn't
    # work with static linking. -lmingwthrd must come BEFORE -lmingw, so
    # it is prepended to QMAKE_LIBS_QT_ENTRY.
    # It can be turned off with MINGW_THREAD_BUGFIX=0, just in case it causes
    # any problems on some untested qmake profile now or in the future.
    DEFINES += _MT BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN
    QMAKE_LIBS_QT_ENTRY = -lmingwthrd $$QMAKE_LIBS_QT_ENTRY
}

macx:HEADERS += qt/macdockiconhandler.h
macx:OBJECTIVE_SOURCES += qt/macdockiconhandler.mm
macx:LIBS += -framework Foundation -framework ApplicationServices -framework AppKit
macx:DEFINES += MAC_OSX MSG_NOSIGNAL=0
macx:ICON = src/qt/res/icons/bitcredit.icns
macx:TARGET = "Bitcredit-Qt"
macx:QMAKE_CFLAGS_THREAD += -pthread
macx:QMAKE_LFLAGS_THREAD += -pthread
macx:QMAKE_CXXFLAGS_THREAD += -pthread
macx:QMAKE_INFO_PLIST = share/qt/Info.plist

# Set libraries and includes at end, to use platform-defined defaults if not overridden
INCLUDEPATH += $$SECP256K1_INCLUDE_PATH $$BOOST_INCLUDE_PATH $$BDB_INCLUDE_PATH $$OPENSSL_INCLUDE_PATH $$QRENCODE_INCLUDE_PATH $$CURL_INCLUDE_PATH
LIBS += $$join(SECP256K1_LIB_PATH,,-L,) $$join(BOOST_LIB_PATH,,-L,) $$join(BDB_LIB_PATH,,-L,) $$join(OPENSSL_LIB_PATH,,-L,) $$join(QRENCODE_LIB_PATH,,-L,) $$join(CURL_LIB_PATH,,-L,)
LIBS += -lssl -lcrypto -ldb_cxx$$BDB_LIB_SUFFIX -lsecp256k1  -lprotobuf -lcurl -lanl
# -lgdi32 has to happen after -lcrypto (see  #681)
windows:LIBS += -lws2_32 -lshlwapi -lmswsock -lole32 -loleaut32 -luuid -lgdi32 -lpthread -static
LIBS += -lboost_system$$BOOST_LIB_SUFFIX -lboost_filesystem$$BOOST_LIB_SUFFIX -lboost_program_options$$BOOST_LIB_SUFFIX -lboost_thread$$BOOST_THREAD_LIB_SUFFIX -lboost_regex$$BOOST_LIB_SUFFIX
windows:LIBS += -lboost_chrono$$BOOST_LIB_SUFFIX

contains(RELEASE, 1) {
    !windows:!macx {
        # Linux: turn dynamic linking back on for c/c++ runtime libraries
        LIBS += -Wl,-Bdynamic
    }
}

!windows:!macx {
    DEFINES += LINUX
    LIBS += -lrt -ldl
}
#QMAKE_EXTRA_COMPILERS += protobuf_cc
system($$QMAKE_LRELEASE -silent $$_PRO_FILE_)
