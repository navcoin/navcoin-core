package=curl_win
$(package)_version=7_48_0
$(package)_download_path=https://github.com/curl/curl/archive/
$(package)_file_name=curl-$($(package)_version).tar.gz
$(package)_sha256_hash=d248f3f9288ae20b8a7e462cb1909a6e67ad5c585040bca64fa2a71d993f3b1b
$(package)_dependencies=openssl

define $(package)_set_vars
  $(package)_config_opts=--with-ssl=$(host_prefix) --with-random=/dev/urandom
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
