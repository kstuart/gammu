#ifndef GAMMU_MMS_DECODERS_H
#define GAMMU_MMS_DECODERS_H

#include <stdio.h>
#include "mms-data.h"

typedef struct _StreamBuffer *SBUFFER;
typedef struct _MMSValue *MMSVALUE;

size_t MMS_DecodeUintVar(SBUFFER stream);
ssize_t DecodeValueLength(SBUFFER stream);
MMSError MMS_DecodeFieldValue(SBUFFER stream, MMSFIELDINFO fi, MMSValue *out);
MMSError MMS_DecodeShortInteger(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeLongInteger(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeInteger(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeExpiry(SBUFFER stream, MMSVALUE out);
MMSVALUEENUM DecodeWellKnownCharset(SBUFFER stream);
MMSError MMS_DecodeEncodedText(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeFromAddress(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeText(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeQuoteText(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeTokenText(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeExtensionMedia(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeConstrainedEncoding(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeWellKnownMedia(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeTypedParameter(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeUntypedParameter(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeParameters(SBUFFER stream, CPTR end, MMSVALUE out);
MMSError MMS_DecodeMediaType(SBUFFER stream, CPTR end, MMSVALUE out);
MMSError MMS_DecodeContentGeneralForm(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeContentType(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeYesNo(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodePriority(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeStatusValue(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeMessageClass(SBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeMessageType(SBUFFER stream, MMSVALUE out);

#endif //GAMMU_MMS_DECODERS_H
