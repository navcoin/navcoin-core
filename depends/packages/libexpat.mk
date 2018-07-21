package=libexpat
$(package)_version=R_2_2_5
$(package)_download_path=http://navcoin.org/depend-sources/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=aa843ff7aed4770d9c5c7e66648f3e0232392c9d45b1bfc0555787de7958886b
$(package)_dependencies=curl

define $(package)_preprocess_cmds
  ./buildconf.sh
endef

define $(package)_config_cmds 
  $($(package)_autoconf) --without-xmlwf --prefix=$($(package)_staging_dir)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
endef
