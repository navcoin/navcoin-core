package=curl
$(package)_version=7.53.1
$(package)_download_path=https://curl.haxx.se/download/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=64f9b7ec82372edb8eaeded0a9cfa62334d8f98abc65487da01188259392911d
$(package)_dependencies=openssl

define $(package)_set_vars
  $(package)_config_opts=--with-ssl=$(host_prefix)
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
