// Minimal PCRE2 wrapper to preserve existing PCRE v1 call sites with tiny diffs
#pragma once

#include <stdint.h>

// Always use 8-bit code units
#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#endif
#include <pcre2.h>

// Public opaque type used by wrapper
typedef pcre2_code regex_code;

// Map commonly used PCRE v1 option names to PCRE2 for source compatibility
#ifndef PCRE_CASELESS
#define PCRE_CASELESS PCRE2_CASELESS
#endif
#ifndef PCRE_MULTILINE
#define PCRE_MULTILINE PCRE2_MULTILINE
#endif
#ifndef PCRE_DOTALL
#define PCRE_DOTALL PCRE2_DOTALL
#endif
#ifndef PCRE_UTF8
#define PCRE_UTF8 PCRE2_UTF
#endif
#ifndef PCRE_UCP
#define PCRE_UCP PCRE2_UCP
#endif

// Wrapper API (new names)
regex_code *regex_compile(const char *pattern, uint32_t options, const char **errorptr, int *erroffset);
int regex_match(regex_code *code,
                const char *subject,
                size_t length,
                size_t start_offset,
                uint32_t options,
                int *ovector,
                size_t ovecsize);
void regex_free(regex_code *code);

// Back-compat typedef and macro shims so existing code can keep using pcre_* names
#ifndef REGEX_NO_PCRE_COMPAT
#define pcre regex_code

// pcre_compile(pattern, options, &error, &erroffset, tables)
#define pcre_compile(pattern, options, errorptr, erroffset, tableptr) \
  regex_compile((pattern), (options), (errorptr), (erroffset))

// pcre_exec(re, extra, subject, len, start, options, ovector, ovecsize)
#define pcre_exec(re, extra, subject, len, start, options, ovector, ovecsize) \
  regex_match((re), (subject), (len), (start), (options), (ovector), (ovecsize))

// pcre_free(re)
#define pcre_free(re) regex_free((re))
#endif


