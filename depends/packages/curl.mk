package=curl
$(package)_version=7.68.0
$(package)_download_path=https://curl.haxx.se/download/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=1dd7604e418b0b9a9077f62f763f6684c1b092a7bc17e3f354b8ad5c964d7358
$(package)_dependencies=openssl
$(package)_patches=fix_lib_order.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/fix_lib_order.patch
endef

define $(package)_set_vars
  $(package)_config_opts=--enable-static --disable-shared --without-libidn2 --with-pic
  $(package)_config_opts_linux=--with-ssl=$(host_prefix)
  $(package)_config_opts_darwin=--without-ssl --with-secure-transport
  $(package)_config_opts_mingw32=--without-ssl --with-schannel --with-random=/dev/urandom
  $(package)_config_opts_x86_64_mingw32=--target=x86_64-w64-mingw32
  $(package)_config_opts_i686_mingw32=--target=i686-w64-mingw32
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
endef
