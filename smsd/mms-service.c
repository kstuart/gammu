#include <malloc.h>
#include <string.h>
#include <dynamic-buffer.h>
#include "mms-service.h"

MMSError MMS_DecodeFieldValue(CDBUFFER stream, MMSFIELDINFO fi, MMSValue *out)
{
	switch(fi->vt) {
		default:
			printf("unsupported codec (%d)\n", fi->vt);
			return MME_NOCODEC;
		case VT_SHORT_INT:
			MMS_DecodeShortInteger(stream, out);
			break;
		case VT_MESSAGE_TYPE:
			MMS_MessageTypeDecode(stream, out);
			break;
		case VT_TEXT:
			MMS_DecodeQuoteText(stream, out);
			break;
		case VT_LONG_INT:
			MMS_DecodeLongInteger(stream, out);
			break;
		case VT_FROM:
			MMS_DecodeFromAddress(stream, out);
			break;
		case VT_ENCODED_STRING:
			MMS_DecodeEncodedText(stream, out);
			break;
		case VT_MESSAGE_CLASS:
			MMS_MessageClassDecode(stream, out);
			break;
		case VT_PRIORITY:
			MMS_PriorityDecode(stream, out);
			break;
		case VT_YESNO:
			MMS_YesNoDecode(stream, out);
			break;
		case VT_CONTENT_TYPE:
			MMS_ContentTypeDecode(stream, out);
			break;
	}

	return MME_NONE;
}

MMSHEADER MMS_CreateHeader(CDBUFFER stream, MMSHEADERS headers, MMSFieldType type, MMSFIELDINFO fi)
{
	MMSError error;
	MMSValue value;
	MMSHEADER header = NULL;

	error = MMS_DecodeFieldValue(stream, fi, &value);
	if(error != MME_NONE)
		return NULL;

	header = MMSHeaders_NewHeader(headers);
	if(!header) {
		puts("failed to create new header");
		return NULL;
	}

	header->id.type = type;
	header->id.info = fi;
	header->value = value;

	return header;
}

int MMS_ParseHeaders(CDBUFFER stream, MMSHEADERS headers)
{
	MMSValue fid;
	MMSFIELDINFO fi = NULL;
	MMSFieldType type = MMS_HEADER;

	while(MMS_DecodeShortInteger(stream, &fid) == MME_NONE) {
		fi = MMSFields_FindByID(fid.v.short_int);
		if(!fi) {
			fi = WSPFields_FindByID(fid.v.short_int);
			type = WSP_HEADER;
		}

		if (!fi) {
			printf("Unknown field (0x%X)\n", fid.v.short_int);
			return -1;
		}

		MMS_CreateHeader(stream, headers, type, fi);
		if(fi->id == MMS_CONTENT_TYPE)
			return 0;
	}

	return 0;
}

void MMS_DumpHeaders(MMSHEADERS headers, DBUFFER buffer)
{
	for(size_t i = 0; i < headers->end; i++) {
		const char *name = headers->entries[i].id.info->name;
		const char *value = MMSValue_AsString(headers->entries[i].id.info->id, &headers->entries[i].value);
		DynaBuf_PutBytes(buffer, name, strlen(name));
		DynaBuf_PutByte(buffer, '=');
		DynaBuf_PutBytes(buffer, value, strlen(value));
		DynaBuf_PutByte(buffer, '\n');
	}
	DynaBuf_PutByte(buffer, 0);
}

MMSError MMS_ParseParts(CDBUFFER stream, MMSPARTS parts)
{
	size_t num_parts = MMS_DecodeUintVar(stream);

	for(int i = 0; i < num_parts; i++)
		MMS_CreatePart(stream, parts);

	return MME_NONE;
}


MMSError MMS_ParseMessage(CDBUFFER stream, MMSMESSAGE *out)
{
	MMSError error;
	MMSMESSAGE msg = MMSMessage_Init();
	if(!msg)
		return MME_MEMORY;

	error = MMS_ParseHeaders(stream, msg->headers);
	if(error != MME_NONE) {
		MMSMessage_Destroy(&msg);
		return error;
	}

	error = MMS_ParseParts(stream, msg->parts);
	if(error != MME_NONE) {
		MMSMessage_Destroy(&msg);
		return error;
	}

	*out = msg;

	return MME_NONE;
}