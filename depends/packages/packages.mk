packages:=boost openssl libevent zeromq curl gmp expat unbound zlib sodium

linux_packages = libseccomp libcap

qt_packages = qrencode

qt_linux_packages:=qt libxcb xcb_proto libXau xproto freetype fontconfig

qt_darwin_packages=qt
qt_mingw32_packages=qt

wallet_packages=bdb

upnp_packages=miniupnpc

darwin_native_packages = native_biplist native_ds_store native_mac_alias

ifneq ($(build_os),darwin)
darwin_native_packages += native_cctools native_cdrkit native_libdmg-hfsplus
endif
