package=curl
$(package)_version=7.53.1
$(package)_download_path=https://curl.haxx.se/download/
$(package)_file_name=curl-$($(package)_version).tar.gz
$(package)_sha256_hash=64f9b7ec82372edb8eaeded0a9cfa62334d8f98abc65487da01188259392911d
$(package)_dependencies=openssl
$(package)_patches=fix_lib_order.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/fix_lib_order.patch
endef

define $(package)_set_vars
  $(package)_config_env=CURL_CFLAG_EXTRAS="-DBUILDING_LIBCURL" 
  $(package)_config_opts=--with-ssl=$(host_prefix) --with-random=/dev/urandom --enable-static --disable-shared
  $(package)_config_opts_x86_64_mingw32=mingw64 --target=x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --build=x86_64-unknown-linux-gnu
  $(package)_config_opts_i686_mingw32=mingw32 --target=i686-w64-mingw32 --host=i686-w64-mingw32 --build=x86_64-unknown-linux-gnu
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
