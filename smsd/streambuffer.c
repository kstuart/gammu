#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <gammu-smsd.h>
#include <gammu-unicode.h>

#include "streambuffer.h"
#include "../libgammu/misc/coding/coding.h"

#define DYNABUF_MAGIC 0x7A7B7C4CU
#define SBUF_DEFAULT_CAPACITY 50

typedef struct _StreamBuffer {
	unsigned magic;
	size_t capacity;
	size_t offset;
	size_t end;
	SBError error;
	void *base;
} StreamBuffer;

SBError SB_LastError(SBUFFER buffer)
{
	assert(buffer);
	return buffer->error;
}

int IsSB(void *ptr)
{
	return ptr != NULL && ((SBUFFER)ptr)->magic == DYNABUF_MAGIC;
}

size_t SBCapacity(SBUFFER buffer)
{
	if(!buffer)
		return 0;

	return buffer->capacity;
}
char *SBBase(SBUFFER buffer)
{
	if(!buffer)
		return NULL;

	return buffer->base;
}

char *SBPtr(SBUFFER buffer)
{
	if(!buffer)
		return NULL;

	return (char*)buffer->base + buffer->offset;
}

char *SBEnd(SBUFFER buffer)
{
	if(!buffer)
		return NULL;

	return (char*)buffer->base + buffer->end;
}

size_t SBUsed(SBUFFER buffer)
{
	if(!buffer)
		return 0;

	return buffer->end;
}

size_t SBOffset(SBUFFER buffer)
{
	if(!buffer)
		return 0;

	return buffer->offset;
}

size_t SBAvailable(SBUFFER buffer)
{
	if(!buffer)
		return 0;

	return buffer->capacity - buffer->offset;
}

SBUFFER SB_InitWithCapacity(size_t initial_capacity)
{
	SBUFFER buffer = (SBUFFER)malloc(sizeof(struct _StreamBuffer));
	if(!buffer)
		return NULL;

	if(initial_capacity == 0)
		initial_capacity = 1;

	buffer->base = malloc(initial_capacity);
	if(!buffer->base) {
		free(buffer);
		return NULL;
	}

	buffer->magic = DYNABUF_MAGIC;
	buffer->capacity = initial_capacity;
	buffer->offset = 0;
	buffer->end = 0;
	buffer->error = SB_ERR_NONE;

	return buffer;
}

SBUFFER SB_Init()
{
	return SB_InitWithCapacity(SBUF_DEFAULT_CAPACITY);
}

void SB_Clear(SBUFFER buffer)
{
	assert(buffer);
	buffer->offset = 0;
	buffer->end = 0;
	buffer->error = SB_ERR_NONE;
}

void SB_Destroy(SBUFFER *buffer)
{
	if(!buffer || *buffer == NULL)
		return;

	free((*buffer)->base);
	free(*buffer);
	*buffer = NULL;
}

ssize_t SB_Grow(SBUFFER buffer, size_t delta)
{
	if(!buffer || delta == 0)
		return 0;

	size_t new_size = buffer->capacity + delta;
	void *new_base = realloc(buffer->base, new_size);
	if(new_base == NULL) {
		buffer->error = SB_ERR_MEMORY;
		return -1;
	}

	buffer->base = new_base;
	buffer->capacity = new_size;

	return new_size;
}

ssize_t SB_ResizeIfNeeded(SBUFFER buffer, size_t offset, size_t count)
{
	assert(buffer);

	ssize_t needed = count + offset;
	if((ssize_t)buffer->capacity < needed)
		return SB_Grow(buffer, needed - buffer->capacity);

	return buffer->capacity;
}

ssize_t SB_Seek(SBUFFER buffer, long offset, int origin)
{
	assert(buffer);
	size_t new_pos;

	switch (origin) {
		case SEEK_SET: new_pos = offset; break;
		case SEEK_CUR: new_pos = buffer->offset + offset; break;
		case SEEK_END: new_pos = buffer->capacity - offset; break;
		default:
			buffer->error = SB_ERR_BADSEEKORIGIN;
			return -1;
	}

	if(new_pos > buffer->end + 1) {
		buffer->error = SB_ERR_BADSEEKOFFSET;
		return -1;
	}
	buffer->offset = new_pos;

	return buffer->offset;
}

ssize_t SB_MinCapacity(SBUFFER buffer, size_t capacity)
{
	assert(buffer);
	return SB_ResizeIfNeeded(buffer, 0, capacity);
}

int SB_Set(SBUFFER buffer, size_t offset, int c, ssize_t count)
{
	assert(buffer != NULL);
	if(count == -1)
		count = buffer->capacity - offset;

	if(SB_ResizeIfNeeded(buffer, offset, count) == -1)
		return -1;

	memset((char*)buffer->base + offset, c, count);
	return 0;
}

ssize_t SB_FindNext(SBUFFER buffer, unsigned char ch)
{
	assert(buffer);
	size_t pos = buffer->offset;

	for(; pos <= buffer->end; pos++)
		if(*((unsigned char*)buffer->base + pos) == ch)
			break;

	return pos > buffer->end ? -1 : pos - buffer->offset;
}

ssize_t SB_CopyUntil(SBUFFER buffer, void *dst, unsigned char ch)
{
	assert(buffer);
	assert(dst);

	ssize_t count = SB_FindNext(buffer, ch);
	if(count == -1)
		return -1;

	if(IsSB(dst))
		SB_PutBytes(dst, SBPtr(buffer), count);
	else
		memcpy(dst, SBPtr(buffer), count);

	return count;
}

// FIXME: This has corner cases
ssize_t SB_Copy(SBUFFER buffer, size_t offset, const void *src, size_t count)
{
	assert(buffer);
	if(SB_ResizeIfNeeded(buffer, offset, count) == -1)
		return -1;

	memcpy((char*)buffer->base + offset, src, count);
	buffer->end = offset + count;

	return buffer->end - offset;
}

ssize_t SB_PutBytes(SBUFFER buffer, const void *src, size_t nbytes)
{
	assert(buffer);
	return SB_Copy(buffer, buffer->end, src, nbytes);
}

ssize_t SB_GetBytes(SBUFFER buffer, void *dest, size_t nbytes)
{
	assert(buffer);
	assert(dest);

	if(nbytes == 0)
		return 0;

	if(buffer->offset + nbytes > buffer->end)
		nbytes = buffer->end - buffer->offset;

	if(nbytes == 0) {
		buffer->error = SB_ERR_EOS;
		return 0;
	}

	memcpy(dest, (char*)buffer->base + buffer->offset, nbytes);
	buffer->offset += nbytes;

	return nbytes;
}

int SB_PutByte(SBUFFER buffer, unsigned char byte)
{
	assert(buffer != NULL);
	return SB_PutBytes(buffer, &byte, 1);
}

unsigned char SB_PeekByte(SBUFFER buffer)
{
	assert(buffer);

	if(buffer->offset > buffer->end) {
		buffer->error = SB_ERR_EOS;
		return 0;
	}

	buffer->error = SB_ERR_NONE;
	unsigned char v = *((unsigned char*)buffer->base + buffer->offset);

	return v;
}

unsigned char SB_NextByte(SBUFFER buffer)
{
	assert(buffer != NULL);
	unsigned char v = SB_PeekByte(buffer);
	if(buffer->error == SB_ERR_NONE)
		buffer->offset++;

	return v;
}

ssize_t SB_Read(SBUFFER buffer, int fd, size_t size) {
	assert(buffer != NULL);
	if (SB_ResizeIfNeeded(buffer, buffer->end, size) == -1)
		return -1;

	ssize_t bytes_read = read(fd, SBEnd(buffer), size);
	if(bytes_read != -1)
		buffer->end += bytes_read;

	return bytes_read;
}

// does not null terminate string
int SB_PutString(SBUFFER buffer, const char *string)
{
	return SB_PutBytes(buffer, string, strlen(string));
}

// does not null terminate string
int SB_PutFormattedString(SBUFFER buffer, const char *fmt, ...)
{
	assert(buffer);
	assert(fmt);

	va_list l1, l2;
	va_start(l1, fmt);
	va_copy(l2, l1);

	SB_MinCapacity(buffer, buffer->end + vsnprintf(NULL, 0, fmt, l1) + 1);
	va_end(l1);

	int written = vsnprintf(SBEnd(buffer), SBAvailable(buffer), fmt, l2);
	va_end(l2);

	if(written > 0)
		buffer->end += written;

	return written;
}

int SB_PutAsEncodedBase64(SBUFFER buffer, void *data, size_t nbytes)
{
	assert(buffer);

	size_t size = ((4 * nbytes / 3) + 3) & ~3u;

	if(SB_ResizeIfNeeded(buffer, buffer->end, size + 1) == -1)
		return -1;

	EncodeBASE64(data, SBEnd(buffer), nbytes);
	if(*(SBEnd(buffer) + size) != 0)
		return -1;

	buffer->end += size;

	return 0;
}

int SB_Truncate(SBUFFER buffer, size_t count)
{
	assert(buffer);
	if (buffer->end - count < buffer->offset)
		return -1;

	buffer->end -= count;
	return 0;
}
