#include <stddef.h>
#include <math.h>
#include <assert.h>
#include "bitstream.h"

void BitReader_SetStart(BitReader *reader, const unsigned char *buffer)
{
  assert(reader != NULL);
  assert(buffer != NULL);

  reader->stream = (unsigned char*)buffer;
  reader->streampos = 0;
  reader->nibbler = 0;
  reader->nibbles = 0;
}

nibbler_t BitReader_ReadBits(BitReader *reader, size_t length)
{
  int i = 0;
  int bytesToRead = 0;
  nibbler_t bitOffset = 0;
  nibbler_t resultMask = (1u << length) - 1;
  nibbler_t result = 0;

  assert(reader != NULL);

  if(length < 1 || length > sizeof(nibbler_t) * 8)
    return -1;

  if (length > reader->nibbles) {
    bytesToRead = ceil(((double)length - reader->nibbles) / 8);
    for (i = 0; i < bytesToRead; i++) {
      reader->nibbler = (reader->nibbler << 8u) | (reader->stream[reader->streampos++] & 0xFFu);
      reader->nibbles += 8;
    }
  }

  bitOffset = reader->nibbles - length;
  result = (reader->nibbler >> bitOffset) & resultMask;
  reader->nibbles -= length;

  return result;
}

int BitReader_GetPosition(BitReader *reader)
{
  assert(reader != NULL);
  return reader->streampos;
}

void BitWriter_SetStart(BitWriter *writer, unsigned char *buffer)
{
  assert(writer != NULL);
  assert(buffer != NULL);

  writer->stream = buffer;
  writer->streampos = 0;
  writer->nibbler = 0;
  writer->nibbles = 0;
}

void BitWriter_WriteBits(BitWriter *writer, nibbler_t value, size_t length)
{
  nibbler_t value_mask;
  nibbler_t merge_length;
  size_t total_length;

  assert(writer != NULL);

  if(length < 1 || length > sizeof(nibbler_t) * 8)
    return;

  value_mask = (1u << length) - 1;
  total_length = length + writer->nibbles;

  if(total_length < 8) {
    writer->nibbler = (writer->nibbler << length) | (value & value_mask);
    writer->nibbles += length;
    return;
  }

  if(writer->nibbles) {
    merge_length = 8 - writer->nibbles;
    value_mask = (1u << merge_length) -1;
    writer->nibbler = (writer->nibbler << merge_length) | ((value >> (length - merge_length)) & value_mask);
    writer->stream[writer->streampos++] = writer->nibbler & 0xFFu;
    length -= merge_length;
  }

  while(length >= 8) {
    length -= 8;
    writer->stream[writer->streampos++] = (value >> length) & 0xFFu;
  }

  writer->nibbler = value & ((1u << length) - 1);
  writer->nibbles = length;
}

void BitWriter_Flush(BitWriter *writer)
{
  assert(writer != NULL);

  if(writer->nibbles)
    writer->stream[writer->streampos++] = writer->nibbler << (8 - writer->nibbles);

  writer->nibbler = 0;
  writer->nibbles = 0;
}

int BitWriter_GetPosition(BitWriter *writer)
{
  assert(writer != NULL);

  return writer->streampos;
}
