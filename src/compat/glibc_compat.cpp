// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/navcoin-config.h>
#endif

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <errno.h>
#include <glob.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>

#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

// Prior to GLIBC_2.14, memcpy was aliased to memmove.
extern "C" void* memmove(void* a, const void* b, size_t c);
extern "C" void* memcpy(void* a, const void* b, size_t c)
{
    return memmove(a, b, c);
}

extern "C" void __chk_fail(void) __attribute__((__noreturn__));
extern "C" FDELT_TYPE __fdelt_warn(FDELT_TYPE a)
{
    if (a >= FD_SETSIZE)
        __chk_fail();
    return a / __NFDBITS;
}
extern "C" FDELT_TYPE __fdelt_chk(FDELT_TYPE) __attribute__((weak, alias("__fdelt_warn")));

#if defined(__i386__) || defined(__arm__)

extern "C" int64_t __udivmoddi4(uint64_t u, uint64_t v, uint64_t* rp);

extern "C" int64_t __wrap___divmoddi4(int64_t u, int64_t v, int64_t* rp)
{
    int32_t c1 = 0, c2 = 0;
    int64_t uu = u, vv = v;
    int64_t w;
    int64_t r;

    if (uu < 0) {
        c1 = ~c1, c2 = ~c2, uu = -uu;
    }
    if (vv < 0) {
        c1 = ~c1, vv = -vv;
    }

    w = __udivmoddi4(uu, vv, (uint64_t*)&r);
    if (c1)
        w = -w;
    if (c2)
        r = -r;

    *rp = r;
    return w;
}
#endif

extern "C" float log2f_old(float x);
#ifdef __i386__
__asm(".symver log2f_old,log2f@GLIBC_2.1");
#elif defined(__amd64__)
__asm(".symver log2f_old,log2f@GLIBC_2.2.5");
#elif defined(__arm__)
__asm(".symver log2f_old,log2f@GLIBC_2.4");
#elif defined(__aarch64__)
__asm(".symver log2f_old,log2f@GLIBC_2.17");
#elif defined(__riscv)
__asm(".symver log2f_old,log2f@GLIBC_2.27");
#endif
extern "C" float __wrap_log2f(float x)
{
    return log2f_old(x);
}

extern "C" double log_old(double x);
#ifdef __i386__
__asm(".symver log_old,log@GLIBC_2.0");
#elif defined(__amd64__)
__asm(".symver log_old,log@GLIBC_2.2.5");
#elif defined(__arm__)
__asm(".symver log_old,log@GLIBC_2.4");
#elif defined(__aarch64__)
__asm(".symver log_old,log@GLIBC_2.17");
#elif defined(__riscv)
__asm(".symver log_old,log@GLIBC_2.27");
#endif

extern "C" double __wrap_log(double x)
{
    return log_old(x);
}

extern "C" double exp_old(double x);
#ifdef __i386__
__asm(".symver exp_old,exp@GLIBC_2.0");
#elif defined(__amd64__)
__asm(".symver exp_old,exp@GLIBC_2.2.5");
#elif defined(__arm__)
__asm(".symver exp_old,exp@GLIBC_2.4");
#elif defined(__aarch64__)
__asm(".symver exp_old,exp@GLIBC_2.17");
#elif defined(__riscv)
__asm(".symver exp_old,exp@GLIBC_2.27");
#endif

extern "C" double __wrap_exp(double x)
{
    return exp_old(x);
}

extern "C" double pow_old(double x, double y);
#ifdef __i386__
__asm(".symver pow_old,pow@GLIBC_2.0");
#elif defined(__amd64__)
__asm(".symver pow_old,pow@GLIBC_2.2.5");
#elif defined(__arm__)
__asm(".symver pow_old,pow@GLIBC_2.4");
#elif defined(__aarch64__)
__asm(".symver pow_old,pow@GLIBC_2.17");
#elif defined(__riscv)
__asm(".symver pow_old,pow@GLIBC_2.27");
#endif

extern "C" double __wrap_pow(double x, double y)
{
    return pow_old(x, y);
}

extern "C" int glob_old(const char * pattern, int flags, int (*errfunc) (const char *epath, int eerrno), glob_t *pglob);
#ifdef __i386__
__asm(".symver glob_old,glob@GLIBC_2.0");
#elif defined(__amd64__)
__asm(".symver glob_old,glob@GLIBC_2.2.5");
#elif defined(__arm__)
__asm(".symver glob_old,glob@GLIBC_2.4");
#elif defined(__aarch64__)
__asm(".symver glob_old,glob@GLIBC_2.17");
#elif defined(__riscv)
__asm(".symver glob_old,glob@GLIBC_2.27");
#endif

extern "C" int __wrap_glob(const char * pattern, int flags, int (*errfunc) (const char *epath, int eerrno), glob_t *pglob)
{
    return glob_old(pattern, flags, errfunc, pglob);
}

#if defined(__i386__) || defined(__arm__)
extern "C" int __wrap_glob64(const char * pattern, int flags, int (*errfunc) (const char *epath, int eerrno), glob_t *pglob)
{
    return glob_old(pattern, flags, errfunc, pglob);
}
#endif

extern "C" int fcntl_old(int fd, int cmd, ...);
#ifdef __i386__
__asm(".symver fcntl_old,fcntl@GLIBC_2.0");
#elif defined(__amd64__)
__asm(".symver fcntl_old,fcntl@GLIBC_2.2.5");
#elif defined(__arm__)
__asm(".symver fcntl_old,fcntl@GLIBC_2.4");
#elif defined(__aarch64__)
__asm(".symver fcntl_old,fcntl@GLIBC_2.17");
#elif defined(__riscv)
__asm(".symver fcntl_old,fcntl@GLIBC_2.27");
#endif

extern "C" int __wrap_fcntl(int fd, int cmd, ...)
{
    va_list ap;
    void *arg;

    va_start (ap, cmd);
    arg = va_arg (ap, void *);
    va_end (ap);

    return fcntl_old(fd, cmd, arg);
}

#if defined(__i386__) || defined(__arm__)
extern "C" int __wrap_fcntl64(int fd, int cmd, ...)
{
    va_list ap;
    void *arg;

    va_start (ap, cmd);
    arg = va_arg (ap, void *);
    va_end (ap);

    return fcntl_old(fd, cmd, arg);
}
#endif

extern "C" int __poll_chk(struct pollfd *fds, nfds_t nfds, int timeout, size_t fdslen)
{
    if(fdslen / sizeof(*fds) < nfds)
        __chk_fail();
    return poll(fds, nfds, timeout);
}

extern "C" void explicit_bzero_old(void *dst, size_t len)
#ifdef __i386__
__asm(".symver explicit_bzero_old,explicit_bzero@GLIBC_2.0");
#elif defined(__amd64__)
__asm(".symver explicit_bzero_old,explicit_bzero@GLIBC_2.2.5");
#elif defined(__arm__)
__asm(".symver explicit_bzero_old,explicit_bzero@GLIBC_2.4");
#elif defined(__aarch64__)
__asm(".symver explicit_bzero_old,explicit_bzero@GLIBC_2.17");
#elif defined(__riscv)
__asm(".symver explicit_bzero_old,explicit_bzero@GLIBC_2.27");
#endif

extern "C" void __wrap_explicit_bzero (void *dst, size_t len)
{
  memset (dst, '\0', len);
  /* Compiler barrier.  */
  asm volatile ("" ::: "memory");
}

extern "C" void __explicit_bzero_chk(void *dst, size_t len, size_t dstlen)
{
    if (__glibc_unlikely(dstlen < len))
        __chk_fail();
    __wrap_explicit_bzero(dst, len);
}

extern "C" int getentropy(void *buf, size_t len)
{
    int pre_errno = errno;
    int ret;
    if (len > 256)
        return (-1);
    do {
        ret = syscall(SYS_getrandom, buf, len, 0);
    } while (ret == -1 && errno == EINTR);

    if (ret != (int)len)
        return (-1);
    errno = pre_errno;
    return (0);
}

#define MUL_NO_OVERFLOW	((size_t)1 << (sizeof(size_t) * 4))

extern "C" void* reallocarray(void *optr, size_t nmemb, size_t size)
{
    if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
            nmemb > 0 && SIZE_MAX / nmemb < size) {
        errno = ENOMEM;
        return NULL;
    }
    return realloc(optr, size * nmemb);
}
