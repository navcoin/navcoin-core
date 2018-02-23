package=unbound
$(package)_version=1.6.8
$(package)_download_path=http://unbound.net/downloads/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash= e3b428e33f56a45417107448418865fe08d58e0e7fea199b855515f60884dd49
$(package)_dependencies=openssl libexpat

define $(package)_set_vars
  $(package)_config_opts=--with-ssl=$(host_prefix) --disable-gost --disable-ecdsa
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
