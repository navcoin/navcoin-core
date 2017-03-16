package=curl_win
$(package)_version=7.44.0
$(package)_download_path=https://github.com/curl/curl/releases/download/curl-7_44_0/
$(package)_file_name=curl-$($(package)_version).tar.gz
$(package)_sha256_hash=d01212ee29110799db969be31eeab902f50eca00d52fb8fb44630d08964762be
$(package)_dependencies=openssl

define $(package)_set_vars
  $(package)_config_opts=--with-ssl=$(host_prefix) --with-random=/dev/urandom
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
