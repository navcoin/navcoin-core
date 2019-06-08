package=curl
$(package)_version=7.64.1
$(package)_download_path=https://curl.haxx.se/download/
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=9252332a7f871ce37bfa7f78bdd0a0e3924d8187cc27cb57c76c9474a7168fb3
$(package)_dependencies=openssl
$(package)_patches=fix_lib_order.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/fix_lib_order.patch
endef

define $(package)_set_vars
  $(package)_config_env=CURL_CFLAG_EXTRAS="-DBUILDING_LIBCURL"
  $(package)_config_opts=--with-ssl=$(host_prefix) --enable-static --disable-shared --without-libidn2 --with-pic
  $(package)_config_opts_mingw32=--with-random=/dev/urandom
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
