/**
 * Compatibility shims to abstract file operations across operating systems
 */

#if defined(WIN32) || defined(_WIN32)
#define DIR_SEPARATOR "\\"
#define DIR_SEPARATOR_CHAR '\\'
#else
#define DIR_SEPARATOR "/"
#define DIR_SEPARATOR_CHAR '/'
#endif
