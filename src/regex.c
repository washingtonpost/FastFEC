#include "regex.h"
#include <string.h>

regex_code *regex_compile(const char *pattern, uint32_t options, const char **errorptr, int *erroffset)
{
  int error_code = 0;
  PCRE2_SIZE error_offset = 0;
  regex_code *code = pcre2_compile((PCRE2_SPTR)pattern,
                                   PCRE2_ZERO_TERMINATED,
                                   options,
                                   &error_code,
                                   &error_offset,
                                   NULL);
  if (code == NULL)
  {
    static char msg[256];
    msg[0] = '\0';
    pcre2_get_error_message(error_code, (PCRE2_UCHAR *)msg, sizeof(msg));
    if (errorptr)
    {
      *errorptr = msg;
    }
    if (erroffset)
    {
      *erroffset = (int)error_offset;
    }
  }
  else
  {
    if (errorptr)
    {
      *errorptr = NULL;
    }
    if (erroffset)
    {
      *erroffset = 0;
    }
  }
  return code;
}

int regex_match(regex_code *code,
                const char *subject,
                size_t length,
                size_t start_offset,
                uint32_t options,
                int *ovector,
                size_t ovecsize)
{
  int rc;
  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(code, NULL);
  if (match_data == NULL)
  {
    return PCRE2_ERROR_NOMEMORY;
  }

  rc = pcre2_match(code,
                   (PCRE2_SPTR)subject,
                   (PCRE2_SIZE)length,
                   (PCRE2_SIZE)start_offset,
                   options,
                   match_data,
                   NULL);

  if (rc >= 0 && ovector != NULL && ovecsize > 0)
  {
    PCRE2_SIZE *ovec = pcre2_get_ovector_pointer(match_data);
    size_t pairs = (size_t)rc + 1; // whole match + captured groups
    size_t want = pairs * 2;
    size_t n = ovecsize < want ? ovecsize : want;
    for (size_t i = 0; i < n; i++)
    {
      ovector[i] = (int)ovec[i];
    }
  }

  pcre2_match_data_free(match_data);
  return rc;
}

void regex_free(regex_code *code)
{
  if (code != NULL)
  {
    pcre2_code_free(code);
  }
}


