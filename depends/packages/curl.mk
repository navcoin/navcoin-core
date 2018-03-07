package=curl
$(package)_version=7.58.0
$(package)_download_path=https://curl.haxx.se/download/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=cc245bf9a1a42a45df491501d97d5593392a03f7b4f07b952793518d97666115
$(package)_dependencies=openssl
$(package)_patches=fix_lib_order.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/fix_lib_order.patch
endef

define $(package)_set_vars
  $(package)_config_env=CURL_CFLAG_EXTRAS="-DBUILDING_LIBCURL"
  $(package)_config_opts=--with-ssl=$(host_prefix)/lib
  $(package)_config_opts_darwin=--enable-static --disable-shared
  $(package)_config_opts_mingw32=--with-random=/dev/urandom --enable-static --disable-shared
  $(package)_config_opts_x86_64_mingw32=mingw64
  $(package)_config_opts_i686_mingw32=mingw32
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
