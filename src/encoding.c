
#include "encoding.h"
#include <stdint.h>

// UTF-8 decoder notice
// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const uint8_t utf8d[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 00..1f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20..3f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 40..5f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 60..7f
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, // 80..9f
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, // a0..bf
    8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // c0..df
    0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3,                 // e0..ef
    0xb, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,                 // f0..ff
    0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, 0x6, 0x1, 0x1, 0x1, 0x1,                 // s0..s0
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, // s1..s2
    1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, // s3..s4
    1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1, // s5..s6
    1, 3, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // s7..s8
};

void collectLineInfo(STRING *line, LINE_INFO *info)
{
  // Initialize info
  info->ascii28 = 0;
  info->asciiOnly = 1;
  info->validUtf8 = 1;
  info->length = 0;
  uint32_t state = UTF8_ACCEPT;
  uint32_t type;

  int i = 0;
  while (1)
  {
    char c = line->str[i];
    i++;
    if (c == 0)
    {
      break;
    }
    info->length++;
    if (c == 28)
    {
      // Has char 28 (separator)
      info->ascii28 = 1;
    }
    if (c > 127)
    {
      // Not ascii only anymore
      info->asciiOnly = 0;
    }
    // Check for valid UTF-8 using DFA
    type = utf8d[(uint8_t)c];
    state = utf8d[256 + state * 16 + type];
    if (state == UTF8_REJECT)
    {
      // Invalid UTF-8
      info->validUtf8 = 0;
    }
  }
}

// Adapted from https://stackoverflow.com/a/4059934
// Ensure the output buffer is twice as long as the input
int iso_8859_1_to_utf_8(STRING *in, STRING *output)
{
  growStringTo(output, in->n * 2 + 1);

  uint8_t *line = (uint8_t *)in->str;
  uint8_t *out = (uint8_t *)output->str;
  int length = 0;
  while (*line)
  {
    if (*line < 128)
    {
      *out++ = *line++;
    }
    else
    {
      *out++ = 0xc2 + (*line > 0xbf), *out++ = (*line++ & 0x3f) + 0x80;
      length += 2;
    }
  }
  return length;
}

int decodeLine(LINE_INFO *info, STRING *in, STRING *output)
{
  // Check line info
  collectLineInfo(in, info);

  if (!info->validUtf8)
  {
    return iso_8859_1_to_utf_8(in, output);
  }
  else
  {
    // Copy memory buffer over
    copyString(in, output);
    return info->length;
  }
}
