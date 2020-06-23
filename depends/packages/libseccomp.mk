package=libseccomp
$(package)_version=2.4.3
$(package)_download_path=https://github.com/seccomp/libseccomp/releases/download/v$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=cf15d1421997fac45b936515af61d209c4fd788af11005d212b3d0fd71e7991d

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --with-pic
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
