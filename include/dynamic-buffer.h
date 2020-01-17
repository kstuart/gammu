#ifndef GAMMU_DYNAMIC_BUFFER_H
#define GAMMU_DYNAMIC_BUFFER_H

#include <stdio.h>

typedef struct _DynamicBuffer *DBUFFER;
typedef struct _DynamicBuffer *CDBUFFER;

typedef enum _DNBError {
	EDNB_OK = 0,
	EDNB_EOS, // end of buffer reached
	EDNB_BADSEEKORIGIN,
	EDNB_BADSEEKOFFSET
} DNBError;

DBUFFER DynaBuf_Init(void);
DBUFFER DynaBuf_InitWithCapacity(size_t initial_capacity);
ssize_t DynaBuf_ResizeIfNeeded(DBUFFER buffer, size_t offset, size_t count);
ssize_t DynaBuf_Grow(DBUFFER buffer, size_t delta);
ssize_t DynaBuf_MinCapacity(DBUFFER buffer, size_t capacity);
void DynaBuf_Destroy(DBUFFER *buffer);
DNBError DynaBuf_LastError(CDBUFFER buffer);

int IsDynaBuf(void *ptr);
size_t DynaBufCapacity(CDBUFFER buffer);
char *DynaBufBase(CDBUFFER buffer);
char *DynaBufPtr(CDBUFFER buffer);
size_t DynaBufUsed(CDBUFFER buffer);
size_t DynaBufOffset(CDBUFFER buffer);
size_t DynaBufAvailable(CDBUFFER buffer);


int DynaBuf_Set(DBUFFER buffer, size_t offset, int c, ssize_t count);
int DynaBuf_Copy(DBUFFER buffer, size_t offset, const void *src, size_t count);
ssize_t DynaBuf_CopyUntil(DBUFFER buffer, void *dst, unsigned char ch);
ssize_t DynaBuf_Seek(DBUFFER buffer, long offset, int origin);
ssize_t DynaBuf_FindNext(DBUFFER buffer, unsigned char ch);

int DynaBuf_PutByte(DBUFFER buffer, unsigned char byte);
int DynaBuf_PutBytes(DBUFFER buffer, const void *src, size_t size);
int DynaBuf_PutString(DBUFFER buffer, const char *string);
int DynaBuf_PutFormattedString(DBUFFER buffer, const char *fmt, ...);

unsigned char DynaBuf_PeekByte(DBUFFER buffer);
unsigned char DynaBuf_NextByte(DBUFFER buffer);

ssize_t DynaBuf_Read(DBUFFER buffer, int fd, size_t size);

DBUFFER DynaBuf_AsUTF8(DBUFFER buffer);


#endif //GAMMU_DYNAMIC_BUFFER_H
