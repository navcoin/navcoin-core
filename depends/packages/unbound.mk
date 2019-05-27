package=unbound
$(package)_version=1.9.0
$(package)_download_path=http://unbound.net/downloads/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=415af94b8392bc6b2c52e44ac8f17935cc6ddf2cc81edfb47c5be4ad205ab917
$(package)_dependencies=openssl expat
$(package)_patches=getentropy.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/getentropy.patch
endef

define $(package)_set_vars
  $(package)_config_opts=--with-ssl=$(host_prefix) --with-libexpat=$(host_prefix) --disable-gost --disable-ecdsa --disable-shared --with-pic
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-all
endef

define $(package)_postprocess_cmds
endef
