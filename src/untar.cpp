// Based on original code by Tim Kientzle, March 2009.

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <boost/filesystem.hpp>

/* Parse an octal number, ignoring leading and trailing nonsense. */
static int
parseoct(const char *p, size_t n)
{
    int i = 0;

    while ((*p < '0' || *p > '7') && n > 0) {
        ++p;
        --n;
    }
    while (*p >= '0' && *p <= '7' && n > 0) {
        i *= 8;
        i += *p - '0';
        ++p;
        --n;
    }
    return (i);
}

/* Returns true if this is 512 zero bytes. */
static int
is_end_of_archive(const char *p)
{
    int n;
    for (n = 511; n >= 0; --n)
        if (p[n] != '\0')
            return (0);
    return (1);
}

/* Create a directory, including parent directories as necessary. */
static void
create_dir(const char *pathname, int mode)
{
    std::string sFile = (GetDataDir(true) / pathname).string();
    fprintf(stdout, "[UNTAR] Creating folder %s\n", sFile.c_str());
    boost::filesystem::path p = sFile;
    boost::filesystem::create_directories(p);
}

/* Create a file, including parent directory as necessary. */
static FILE *
create_file(char *pathname, int mode)
{
    FILE *f;
    std::string sFile = (GetDataDir(true) / pathname).string();
    f = fopen(sFile.c_str(), "wb+");
    fprintf(stdout, "[UNTAR] Creating file %s\n", sFile.c_str());
    if (f == NULL) {
        /* Try creating parent dir and then creating file. */
        char *p = strrchr(pathname, '/');
        if (p != NULL) {
            *p = '\0';
            create_dir(pathname, 0755);
            *p = '/';
            f = fopen(sFile.c_str(), "wb+");
        }
    }
    return (f);
}

/* Verify the tar checksum. */
static int
verify_checksum(const char *p)
{
    int n, u = 0;
    for (n = 0; n < 512; ++n) {
        if (n < 148 || n > 155)
            /* Standard tar checksum adds unsigned bytes. */
            u += ((unsigned char *)p)[n];
        else
            u += 0x20;

    }
    return (u == parseoct(p + 148, 8));
}

/* Extract a tar archive. */
bool
untar(FILE *a, const char *path)
{
    char buff[512];
    FILE *f = NULL;
    size_t bytes_read;
    int filesize;

    fprintf(stdout, "[UNTAR] Extracting from %s\n", path);
    for (;;) {
        bytes_read = fread(buff, 1, 512, a);
        if (bytes_read < 512) {
            fprintf(stderr, "[UNTAR] Short read on %s: expected 512, got %d\n",
                      path, (int)bytes_read);
            return false;
        }
        if (is_end_of_archive(buff)) {
            return true;
        }
        if (!verify_checksum(buff)) {
            fprintf(stderr, "[UNTAR] Checksum failure\n");
            return false;
        }
        filesize = parseoct(buff + 124, 12);
        switch (buff[156]) {
        case '1':
            // Ignoring hardlink
            break;
        case '2':
            // Ignoring symlink
            break;
        case '3':
            // Ignoring character device
            break;
        case '4':
            // Ignoring block device
            break;
        case '5':
            // Extracting dir
            create_dir(buff, parseoct(buff + 100, 8));
            filesize = 0;
            break;
        case '6':
            // Ignoring FIFO
            break;
        default:
            // Extracting file
            if(!strcmp(buff, "wallet.dat")) {
                fprintf(stderr, "[UNTAR] Wrong bootstrap, it includes a wallet.dat file\n");
            } else {
                f = create_file(buff, parseoct(buff + 100, 8));
            }
            break;
        }
        while (filesize > 0) {
            bytes_read = fread(buff, 1, 512, a);
            if (bytes_read < 512) {
                fprintf(stderr, "[UNTAR] Short read on %s: Expected 512, got %d\n",
                          path, (int)bytes_read);
                return false;
            }
            if (filesize < 512)
                bytes_read = filesize;
            if (f != NULL) {
                if (fwrite(buff, 1, bytes_read, f)
                        != bytes_read)
                {
                    fprintf(stderr, "[UNTAR] Failed write\n");
                    fclose(f);
                    f = NULL;
                    return false;
                }
            }
            filesize -= bytes_read;
        }
        if (f != NULL) {
            fclose(f);
            f = NULL;
        }
    }
    return true;
}

