#include <string.h>
#include <assert.h>
#include <gammu-smsd.h>
#include <gammu-unicode.h>
#include <dynamic-buffer.h>

#define DYNABUF_MAGIC 0x7A7B7C4CU
#define DYNABUF_DEFAULT_CAPACITY 50


//---

typedef struct _DynamicBuffer {
	unsigned magic;
	size_t capacity;
	size_t offset;
	DNBError error;
	void *base;
} DynamicBuffer;

DNBError DynaBuf_LastError(CDBUFFER buffer)
{
	assert(buffer);
	return buffer->error;
}

int IsDynaBuf(void *ptr)
{
	return ptr != NULL && ((CDBUFFER)ptr)->magic == DYNABUF_MAGIC;
}

size_t DynaBufCapacity(CDBUFFER buffer)
{
	if(buffer == NULL)
		return 0;

	return buffer->capacity;
}
char *DynaBufBase(CDBUFFER buffer)
{
	if(buffer == NULL)
		return NULL;

	return buffer->base;
}

char *DynaBufPtr(CDBUFFER buffer)
{
	if(buffer == NULL)
		return NULL;

	return (char*)buffer->base + buffer->offset;
}

size_t DynaBufUsed(CDBUFFER buffer)
{
	if(buffer == NULL)
		return 0;

	return buffer->offset;
}

size_t DynaBufOffset(CDBUFFER buffer)
{
	if(buffer == NULL)
		return 0;

	return buffer->offset;
}

size_t DynaBufAvailable(CDBUFFER buffer)
{
	if(buffer == NULL)
		return 0;

	return buffer->capacity - buffer->offset;
}

DBUFFER DynaBuf_InitWithCapacity(size_t initial_capacity)
{
	DBUFFER buffer = (DBUFFER)malloc(sizeof(struct _DynamicBuffer));
	if(buffer == NULL)
		return NULL;

	if(initial_capacity == 0)
		initial_capacity = 1;

	buffer->base = malloc(initial_capacity);
	if(buffer->base == NULL) {
		free(buffer);
		return NULL;
	}

	buffer->magic = DYNABUF_MAGIC;
	buffer->capacity = initial_capacity;
	buffer->offset = 0;

	return buffer;
}

DBUFFER DynaBuf_Init()
{
	return DynaBuf_InitWithCapacity(DYNABUF_DEFAULT_CAPACITY);
}

void DynaBuf_Destroy(DBUFFER *buffer)
{
	if(buffer == NULL || *buffer == NULL)
		return;

	free((*buffer)->base);
	free(*buffer);
	*buffer = NULL;
}

ssize_t DynaBuf_ResizeIfNeeded(DBUFFER buffer, size_t offset, size_t count)
{
	ssize_t needed = count + offset;
	if((ssize_t)buffer->capacity < needed) {
		if(DynaBuf_Grow(buffer, needed - buffer->capacity) == -1)
			return -1;
	}

	return buffer->capacity;
}

ssize_t DynaBuf_Seek(DBUFFER buffer, long offset, int origin)
{
	assert(buffer != NULL);
	size_t new_pos;

	switch (origin) {
		case SEEK_SET: new_pos = offset; break;
		case SEEK_CUR: new_pos = buffer->offset + offset; break;
		case SEEK_END: new_pos = buffer->capacity - offset; break;
		default:
			buffer->error = EDNB_BADSEEKORIGIN;
			return -1;
	}

	if(new_pos > buffer->capacity) {
		buffer->error = EDNB_BADSEEKOFFSET;
		return -1;
	}
	buffer->offset = new_pos;

	return buffer->offset;
}

ssize_t DynaBuf_Grow(DBUFFER buffer, size_t delta)
{
	if(buffer == NULL || delta == 0)
		return 0;

	size_t new_size = buffer->capacity + delta;
	void *new_base = realloc(buffer->base, new_size);
	if(new_base == NULL)
		return -1;

	buffer->base = new_base;
	buffer->capacity = new_size;

	return new_size;
}

ssize_t DynaBuf_MinCapacity(DBUFFER buffer, size_t capacity)
{
	assert(buffer != NULL);
	return DynaBuf_ResizeIfNeeded(buffer, 0, capacity);
}

// doesn't change ptr
int DynaBuf_Set(DBUFFER buffer, size_t offset, int c, ssize_t count)
{
	assert(buffer != NULL);
	if(count == -1)
		count = buffer->capacity - offset;

	if(DynaBuf_ResizeIfNeeded(buffer, offset, count) == -1)
		return -1;

	memset((char*)buffer->base + offset, c, count);
	return 0;
}

int DynaBuf_Copy(DBUFFER buffer, size_t offset, const void *src, size_t count)
{
	assert(buffer != NULL);
	if(DynaBuf_ResizeIfNeeded(buffer, offset, count) == -1)
		return -1;

	memcpy((char*)buffer->base + offset, src, count);
	buffer->offset = offset + count;

	return 0;
}

ssize_t DynaBuf_FindNext(DBUFFER buffer, unsigned char ch)
{
	assert(buffer != NULL);
	size_t pos = buffer->offset;

	for(; pos <= buffer->capacity; pos++)
		if(*((unsigned char*)buffer->base + pos) == ch)
			break;

	return pos > buffer->capacity ? -1 : pos - buffer->offset;
}

ssize_t DynaBuf_CopyUntil(DBUFFER buffer, void *dst, unsigned char ch)
{
	assert(buffer != NULL);
	assert(dst != NULL);

	ssize_t count = DynaBuf_FindNext(buffer, ch);
	if(count == -1)
		return -1;

	count++; // inclusive

	if(IsDynaBuf(dst))
		DynaBuf_PutBytes(dst, DynaBufPtr(buffer), count);
	else
		memcpy(dst, DynaBufPtr(buffer), count);

	return count;
}

int DynaBuf_PutBytes(DBUFFER buffer, const void *chunk, size_t size)
{
	assert(buffer != NULL);
	return DynaBuf_Copy(buffer, buffer->offset, chunk, size);
}

size_t DynaBuf_GetBytes(DBUFFER buffer, void *destination, size_t size)
{
	assert(buffer != NULL);
	assert(destination != NULL);

	if(size == 0)
		return 0;

	if(buffer->offset + size > buffer->capacity)
		size = buffer->capacity - buffer->offset;

	if(size == 0) {
		buffer->error = EDNB_EOS;
		return 0;
	}

	memcpy(destination, (char*)buffer->base + buffer->offset, size);
	buffer->offset += size;

	return size;
}

int DynaBuf_PutByte(DBUFFER buffer, const unsigned char byte)
{
	assert(buffer != NULL);
	return DynaBuf_PutBytes(buffer, &byte, 1);
}

unsigned char DynaBuf_PeekByte(DBUFFER buffer)
{
	assert(buffer != NULL);
	if(buffer->offset > buffer->capacity) {
		buffer->error = EDNB_EOS;
		return 0;
	}

	unsigned char v = *((unsigned char*)buffer->base + buffer->offset);

	return v;
}

unsigned char DynaBuf_NextByte(DBUFFER buffer)
{
	assert(buffer != NULL);
	unsigned char v = DynaBuf_PeekByte(buffer);
	buffer->offset++;

	return v;
}

ssize_t DynaBuf_Read(DBUFFER buffer, int fd, size_t size) {
	assert(buffer != NULL);
	if (DynaBuf_ResizeIfNeeded(buffer, buffer->offset, size) == -1)
		return -1;

	ssize_t bytes_read = read(fd, DynaBufPtr(buffer), size);
	if(bytes_read != -1)
		buffer->offset += bytes_read;

	return bytes_read;
}

#include <stdarg.h>

#define FMT_STRING_MAX_SIZE (65536)

// does not null terminate string
int DynaBuf_PutString(DBUFFER buffer, const char *str)
{
	return DynaBuf_PutBytes(buffer, str, strlen(str));
}

// does not null terminate string
int DynaBuf_PutFormattedString(DBUFFER buffer, const char *fmt, ...)
{
	char buf[FMT_STRING_MAX_SIZE];
	int written;
	va_list	argp;

	va_start(argp, fmt);
	written = vsprintf(buf, fmt, argp);
	va_end(argp);

	return DynaBuf_PutBytes(buffer, buf, written);
}

DBUFFER DynaBuf_AsUTF8(DBUFFER buffer)
{
	size_t len = buffer->offset;
	DBUFFER buf = DynaBuf_InitWithCapacity(len);
	EncodeUnicode(buf->base, buffer->base, len);
//	if( == FALSE)
//		DynaBuf_Destroy(&buf);

	buf->offset = len;

	return buf;
}