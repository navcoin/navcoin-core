package=bdb
$(package)_version=5.3.28
$(package)_download_path=https://download.oracle.com/berkeley-db
$(package)_file_name=db-$($(package)_version).tar.gz
$(package)_sha256_hash=e0a992d740709892e81f9d93f06daf305cf73fb81b545afe72478043172c3628
$(package)_build_subdir=build_unix

define $(package)_set_vars
$(package)_config_opts=--disable-shared --enable-cxx --disable-replication --with-cryptography
$(package)_config_opts_mingw32=--enable-mingw
$(package)_config_opts_linux=--with-pic
$(package)_cflags+=-Wno-error=implicit-function-declaration
$(package)_cxxflags=-std=c++17
$(package)_cppflags_mingw32=-DUNICODE -D_UNICODE
endef

define $(package)_preprocess_cmds
  cd src &&\
  sed -i.old 's/__atomic_compare_exchange/__atomic_compare_exchange_db/' dbinc/atomic.h && \
  sed -i.old 's/atomic_init/atomic_init_db/' dbinc/atomic.h mp/mp_region.c mp/mp_mvcc.c mp/mp_fget.c mutex/mut_method.c mutex/mut_tas.c && \
  sed -i.old 's/WinIoCtl.h/winioctl.h/' dbinc/win_db.h &&\
  cd .. &&\
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub dist
endef

define $(package)_config_cmds
  ../dist/$($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) -j$(JOBS) libdb_cxx-5.3.a libdb-5.3.a
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install_lib install_include
endef
