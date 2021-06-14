packages:=boost openssl libevent zeromq curl gmp expat unbound zlib sodium

linux_packages = libseccomp libcap

qt_packages = qrencode

qt_linux_packages:=qt libxcb xcb_proto libXau xproto freetype fontconfig libxkbcommon

qt_darwin_packages=qt
qt_mingw32_packages=qt

wallet_packages=bdb

upnp_packages=miniupnpc

darwin_native_packages = native_ds_store native_mac_alias

$(host_arch)_$(host_os)_native_packages += native_b2

ifneq ($(build_os),darwin)
darwin_native_packages += native_cctools native_libtapi native_libdmg-hfsplus

ifeq ($(strip $(FORCE_USE_SYSTEM_CLANG)),)
darwin_native_packages+= native_clang
endif

endif
