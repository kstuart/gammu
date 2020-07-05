#ifndef GAMMU_STREAMBUFFER_H
#define GAMMU_STREAMBUFFER_H

#include <stdio.h>
#include <stdbool.h>

typedef struct _StreamBuffer *SBUFFER;

typedef enum _SBBError {
	SB_ERR_NONE = 0,
	SB_ERR_EOS, // end of buffer reached
	SB_ERR_BADSEEKORIGIN,
	SB_ERR_BADSEEKOFFSET,
	SB_ERR_MEMORY,
	SB_ERR_MAPPED,
} SBError;

bool IsSB(void *ptr);
bool SB_IsMapped(SBUFFER b);

SBUFFER SB_Init(void);
SBUFFER SB_InitWithCapacity(size_t initial_capacity);
SBUFFER SB_MapBuffer(unsigned char *buffer, size_t length);

ssize_t SB_ResizeIfNeeded(SBUFFER buffer, size_t offset, size_t count);
ssize_t SB_Grow(SBUFFER buffer, size_t delta);
ssize_t SB_MinCapacity(SBUFFER buffer, size_t capacity);
void SB_Clear(SBUFFER buffer);
void SB_Destroy(SBUFFER *buffer);
SBError SB_LastError(SBUFFER buffer);

size_t SBCapacity(SBUFFER buffer);
char *SBBase(SBUFFER buffer);
char *SBEnd(SBUFFER buffer);
char *SBPtr(SBUFFER buffer);
size_t SBUsed(SBUFFER buffer);
size_t SBOffset(SBUFFER buffer);
size_t SBAvailable(SBUFFER buffer);

int SB_Set(SBUFFER buffer, size_t offset, int c, ssize_t count);
ssize_t SB_Copy(SBUFFER buffer, size_t offset, const void *src, size_t count);
ssize_t SB_CopyUntil(SBUFFER buffer, void *dst, unsigned char ch);
ssize_t SB_Seek(SBUFFER buffer, long offset, int origin);
ssize_t SB_FindNext(SBUFFER buffer, unsigned char ch);
ssize_t SB_ReadBytesAvailable(SBUFFER buffer);

int SB_PutByte(SBUFFER buffer, unsigned char byte);
ssize_t SB_PutBytes(SBUFFER buffer, const void *src, size_t nbytes);
int SB_PutAsEncodedBase64(SBUFFER buffer, void *data, size_t nbytes);
int SB_PutAsBinHex(SBUFFER buffer, void *data, size_t nbytes);
int SB_PutString(SBUFFER buffer, const char *string);
int SB_PutFormattedString(SBUFFER buffer, const char *fmt, ...);
ssize_t SB_GetBytes(SBUFFER buffer, void *dest, size_t nbytes);

unsigned char SB_PeekByte(SBUFFER buffer);
unsigned char SB_NextByte(SBUFFER buffer);

int SB_Truncate(SBUFFER buffer, size_t count);
ssize_t SB_Read(SBUFFER buffer, int fd, size_t size);
ssize_t SB_SaveToOpenFile(SBUFFER buffer, int fd);
ssize_t SB_SaveToFile(SBUFFER buffer, const char *filename);

#endif //GAMMU_STREAMBUFFER_H
