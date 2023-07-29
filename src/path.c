#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include "compat.h"

#ifndef PATH_MAX_LENGTH
#define PATH_MAX_LENGTH 4096 /* # chars in a path name including nul */
#endif
#ifndef EEXIST
#define EEXIST 17
#endif
#ifndef ENAMETOOLONG
#define ENAMETOOLONG 63
#endif

// From https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
int mkdir_safe(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX_LENGTH];
    char *p;

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path) - 1)
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++)
    {
        if (*p == DIR_SEPARATOR_CHAR)
        {
            /* Temporarily truncate */
            *p = '\0';

#if defined(_WIN32)
            int mkdirResult = mkdir(_path);
#else
            int mkdirResult = mkdir(_path, S_IRWXU);
#endif
            if (mkdirResult != 0)
            {
                if (errno != EEXIST)
                    return -1;
            }

            *p = DIR_SEPARATOR_CHAR;
        }
    }

#if defined(_WIN32)
    int mkdirResult = mkdir(_path);
#else
    int mkdirResult = mkdir(_path, S_IRWXU);
#endif
    if (mkdirResult != 0)
    {
        if (errno != EEXIST)
            return -1;
    }

    return 0;
}

char *pathJoin(const char *path1, const char *path2)
{
    size_t len1 = strlen(path1);
    size_t len2 = strlen(path2);
    char *result = malloc(len1 + len2 + 2); // +1 for the separator, +1 for the null terminator
    strcpy(result, path1);
    if (result[len1 - 1] != DIR_SEPARATOR_CHAR)
    {
        result[len1] = DIR_SEPARATOR_CHAR;
        len1++;
    }
    strcpy(result + len1, path2);
    return result;
}
