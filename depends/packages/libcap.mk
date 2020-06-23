package=libcap
$(package)_version=2.36
$(package)_download_path=https://mirrors.edge.kernel.org/pub/linux/libs/security/linux-privs/libcap2/
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=5048c849bdbbe24d2ca59463142cb279abec5edf3ab6731ab35a596bcf538a49

define $(package)_build_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir)$(host_prefix) prefix=/ lib="lib" install && \
      rm $($(package)_staging_dir)$(host_prefix)/lib/libcap.so*
endef
