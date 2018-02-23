package=unbound
$(package)_version=1.6.8
$(package)_download_path=http://unbound.net/downloads/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash= e3b428e33f56a45417107448418865fe08d58e0e7fea199b855515f60884dd49
$(package)_dependencies=openssl
$(package)_patches=fix_pkg_config.patch
$(package)_deptrack=1
$(package)_cppflags=-I/usr/include

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/fix_pkg_config.patch
endef

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
