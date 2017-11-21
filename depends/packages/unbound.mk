package=unbound
$(package)_version=1.6.7
$(package)_download_path=http://unbound.net/downloads/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=4e7bd43d827004c6d51bef73adf941798e4588bdb40de5e79d89034d69751c9f
$(package)_dependencies=openssl

define $(package)_preprocess_cmds
endef

define $(package)_set_vars
  $(package)_config_opts=--with-ssl=$(host_prefix)/lib
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
