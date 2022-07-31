#include "buffer.h"
#include <string.h>

BUFFER *newBuffer(int bufferSize, BufferRead bufferRead)
{
  BUFFER *buffer = malloc(sizeof(BUFFER));
  buffer->bufferSize = bufferSize;
  buffer->bufferPos = 0;
  buffer->buffer = malloc(bufferSize);
  buffer->streamStarted = 0;
  buffer->bufferRead = bufferRead;
  return buffer;
}

void freeBuffer(BUFFER *buffer)
{
  free(buffer->buffer);
  free(buffer);
}

size_t readBuffer(char *buffer, int want, FILE *file)
{
  return fread(buffer, 1, want, file);
}

size_t fillBuffer(BUFFER *buffer, void *data)
{
  // Fill the buffer
  buffer->bufferPos = 0;
  int bytesRead = buffer->bufferRead(buffer->buffer, buffer->bufferSize, data);
  buffer->bufferSize = bytesRead;
  return bytesRead;
}

int readLine(BUFFER *buffer, STRING *string, void *data)
{
  int eof = 0;
  // Start stream if necessary
  if (!buffer->streamStarted)
  {
    if (fillBuffer(buffer, data) == 0)
    {
      // Check for eof
      eof = 1;
    }
    buffer->streamStarted = 1;
  }

  // Read through the buffer character by character to fill a line.
  int n = 0;
  int stringStart = 0;
  int start = buffer->bufferPos;
  while (1)
  {
    if (buffer->bufferPos >= buffer->bufferSize)
    {
      // Flush memcpy to string buffer
      memcpy(string->str + stringStart, buffer->buffer + start, buffer->bufferPos - start);
      stringStart += buffer->bufferPos - start;

      // Buffer needs to be refilled
      if (fillBuffer(buffer, data) == 0)
      {
        // End of file
        eof = 1;
      }
      start = buffer->bufferPos;
    }
    char c = eof ? '\0' : buffer->buffer[buffer->bufferPos];
    buffer->bufferPos++;
    // Set the string
    while (n + 2 > string->n)
    {
      // Ensure the string is large enough
      growString(string);
    }
    int end = c == '\n';
    if (end)
    {
      n++;
    }
    if (end || eof)
    {
      string->str[n] = '\0';
    }
    n++;

    if (end || eof)
    {
      memcpy(string->str + stringStart, buffer->buffer + start, buffer->bufferPos - start - 1 + (end ? 1 : 0));
      break;
    }
  }
  return n - 1;
}
