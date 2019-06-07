#ifndef GAMMU_BITSTREAM_H
#define GAMMU_BITSTREAM_H

struct Bitstream {
  unsigned char* stream;
  int streampos;
  unsigned nibbler;
  size_t nibbles;
};

typedef struct Bitstream BitReader;
typedef struct Bitstream BitWriter;

void BitReader_SetStart(BitReader *reader, const unsigned char *buffer);
unsigned BitReader_ReadBits(BitReader *reader, size_t length);
int BitReader_GetPosition(BitReader *reader);

void BitWriter_SetStart(BitWriter *writer, unsigned char *buffer);
void BitWriter_WriteBits(BitWriter *writer, unsigned value, size_t length);
void BitWriter_Flush(BitWriter *writer);
int BitWriter_GetPosition(BitWriter *writer);

#endif //GAMMU_BITSTREAM_H
