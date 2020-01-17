#include <malloc.h>
#include <string.h>
#include <limits.h>
#include <dynamic-buffer.h>
#include <time.h>

#include "mms-data.h"
#include "mms-decoders.h"
#include "mms-tables.h"

typedef enum _IntToStringFMT {
	FMT_DECIMAL,
	FMT_HEX
} IntToStringFmt;

const char *IntToStr(unsigned long i, IntToStringFmt fmt)
{
	static char buf[(CHAR_BIT * sizeof(unsigned long) - 1) / 3 + 2];
	switch(fmt) {
		case FMT_DECIMAL:
			snprintf(buf, sizeof(buf), "%zu", i);
			break;
		case FMT_HEX:
			snprintf(buf, sizeof(buf), "%#lx", i);
	}
	return buf;
}

// MMSValue

CSTR MMSValue_AsString(int field_id, MMSValue *v)
{
	char *date;
	switch (v->type) {
		default:
			return "<NOT-CONVERTED-PLEASE-REPORT>";
		case VT_SHORT_INT:
			switch (field_id) {
				default:
					return IntToStr(v->v.short_int, FMT_DECIMAL);
				case MMS_MMS_VERSION:
					return IntToStr(v->v.short_int, FMT_HEX);
			}
		case VT_LONG_INT:
			switch (field_id) {
				default:
					return IntToStr(v->v.long_int, FMT_DECIMAL);
				case MMS_DATE:
					date = asctime(gmtime((time_t *) &v->v.long_int));
					date[strlen(date) - 1] = 0;
					return date;
			}
		case VT_TEXT:
			return v->v.str;
		case VT_ENCODED_STRING:
			return v->v.encoded_string.string;
		case VT_ENUM:
		case VT_YESNO:
		case VT_MESSAGE_TYPE:
		case VT_MESSAGE_CLASS:
		case VT_HIDESHOW:
		case VT_RESPONSE_STATUS:
		case VT_PRIORITY:
			return v->v.enum_v->name;
	}
}

MMSError MMSValue_SetEnum(MMSVALUE out, MMSValueType vt, MMSVALUEENUM entry)
{
	out->allocated = 0;
	out->type = vt;
	out->v.enum_v = entry;
	return MME_NONE;
}

// MMSHeader

void MMSHeader_Clear(MMSHEADER header)
{
	if(!header->value.allocated)
		return;

	free(header->value.v.ptr);
	header->value.v.ptr = NULL;
}

// MMSHeaders

int MMSHeaders_GrowNumEntries(MMSHEADERS headers, int delta)
{
	MMSHEADER entries = realloc(headers->entries, (headers->capacity + delta) * sizeof(MMSHeader));
	if(entries) {
		memset(entries + headers->capacity, 0, delta * sizeof(MMSHeader));
		headers->capacity += delta;
		headers->entries = entries;
	}

	return headers->capacity;
}

MMSHEADERS MMSHeaders_InitWithCapacity(int num_headers)
{
	MMSHEADERS headers = malloc(sizeof(MMSHeaders));
	if(!headers)
		return NULL;

	memset(headers, 0, sizeof(MMSHeaders));
	if(MMSHeaders_GrowNumEntries(headers, num_headers) != num_headers) {
		free(headers);
		return NULL;
	}

	return headers;
}

void MMSHeaders_Destroy(MMSHEADERS *headers)
{
	for(size_t i =0; i < (*headers)->end; i++)
		MMSHeader_Clear(&(*headers)->entries[i]);

	free((*headers)->entries);
	free(*headers);
	*headers = NULL;
}

MMSHEADER MMSHeaders_NewHeader(MMSHEADERS headers)
{
	if(headers->end == headers->capacity)
		MMSHeaders_GrowNumEntries(headers, (int)(headers->capacity * 0.5));

	if(headers->end >= headers->capacity)
		return NULL;

	return &headers->entries[headers->end++];
}

MMSHEADER MMSHeader_FindByID(MMSHEADERS headers, MMSFieldType type, int fieldId)
{
	MMSHEADER header;

	for(int i = 0; i < headers->end; i++) {
		header = &headers->entries[i];
		if (header->id.type == type && header->id.info->id == fieldId)
			return header;
	}

	return NULL;
}

// MMSPart

int MMSParts_GrowNumEntries(MMSPARTS parts, int delta)
{
	MMSPART entries = realloc(parts->entries, (parts->capacity + delta) * sizeof(MMSPart));
	if(entries) {
		memset(entries + parts->capacity, 0, delta * sizeof(MMSPart));
		parts->capacity += delta;
		parts->entries = entries;
	}

	return parts->capacity;
}

MMSPARTS MMSParts_InitWithCapacity(int num_parts)
{
	MMSPARTS parts = malloc(sizeof(MMSParts));
	if(!parts)
		return NULL;

	memset(parts, 0, sizeof(MMSParts));
	if(MMSParts_GrowNumEntries(parts, num_parts) != num_parts) {
		free(parts);
		return NULL;
	}

	return parts;
}

void MMSPart_Destroy(MMSPART *part)
{
	MMSHeaders_Destroy(&(*part)->headers);
	*part = NULL; // NOTE: no free?
}

void MMSParts_Destroy(MMSPARTS *parts)
{
	MMSPART part;
	for(size_t i =0; i < (*parts)->end; i++) {
		part = &(*parts)->entries[i];
		MMSPart_Destroy(&part);
	}

	free((*parts)->entries);
	free(*parts);
	*parts = NULL;
}

MMSPART MMSParts_NewPart(MMSPARTS parts)
{
	MMSPART part;
	if(parts->end == parts->capacity)
		MMSParts_GrowNumEntries(parts, (int)(parts->capacity * 0.5));

	if(parts->end >= parts->capacity)
		return NULL;

	part = &parts->entries[parts->end++];
	part->headers = MMSHeaders_InitWithCapacity(2);
	return part;
}

MMSPART MMS_CreatePart(CDBUFFER stream, MMSPARTS parts)
{
	MMSError error;
	size_t headers_len = MMS_DecodeUintVar(stream);
	size_t data_len = MMS_DecodeUintVar(stream);

	size_t temp_mark = DynaBufOffset(stream);

	MMSValue v;
	error = MMS_ContentTypeDecode(stream, &v);
	if(error != MME_NONE)
		return NULL;

	MMSPART part = MMSParts_NewPart(parts);
	if(!part)
		return NULL;

	MMSHEADER header = MMSHeaders_NewHeader(part->headers);
	header->id.type = MMS_HEADER;
	header->id.info = MMSFields_FindByID(MMS_CONTENT_TYPE);
	header->value = v;

	// TODO: parse remaining headers once content type decoding is complete, skip for now.
	DynaBuf_Seek(stream, (long)(temp_mark + headers_len), SEEK_SET);
	part->data_len = data_len;
	part->data = DynaBufPtr(stream);

	DynaBuf_Seek(stream, data_len, SEEK_CUR);

	return part;
}

MMSMESSAGE MMSMessage_Init()
{
	MMSMESSAGE msg = malloc(sizeof(MMSMessage));
	if(!msg)
		return NULL;

	memset(msg, 0, sizeof(MMSMessage));
	msg->headers = MMSHeaders_InitWithCapacity(10);
	if(!msg->headers) {
		free(msg);
		return NULL;
	}

	msg->parts = MMSParts_InitWithCapacity(3);
	if(!msg->parts) {
		MMSHeaders_Destroy(&msg->headers);
		free(msg);
		return NULL;
	}

	return msg;
}

void MMSMessage_Destroy(MMSMESSAGE *message)
{
	MMSHeaders_Destroy(&(*message)->headers);
	MMSParts_Destroy(&(*message)->parts);
}

