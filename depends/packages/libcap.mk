package=libcap
$(package)_version=2.36
$(package)_download_path=https://mirrors.edge.kernel.org/pub/linux/libs/security/linux-privs/libcap2/
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=5048c849bdbbe24d2ca59463142cb279abec5edf3ab6731ab35a596bcf538a49
$(package)_patches=make_rules.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/make_rules.patch
endef

define $(package)_build_cmds
  CC="$($(host_arch)_$(host_os)_CC)" $(MAKE) GOLANG=no DESTDIR=$($(package)_staging_dir) prefix=$(host_prefix) lib="lib" install && \
      rm $($(package)_staging_dir)$(host_prefix)/lib/libcap.so*
endef
