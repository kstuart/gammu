#ifndef GAMMU_MMS_SERVICE_H
#define GAMMU_MMS_SERVICE_H

#include <gammu-error.h>
#include <gammu-smsd.h>

#include "mms-data.h"
#include "mms-tables.h"
#include "mms-decoders.h"
#include "mms-encoders.h"

typedef struct _MMSConveyor {
	GSM_Error (*FetchMMS)(GSM_SMSDConfig*, SBUFFER, GSM_MMSIndicator*);
	GSM_Error (*SendMMS)(GSM_SMSDConfig*, SBUFFER);
} MMSConveyor;
typedef MMSConveyor *MMSCONVEYOR;

LocalTXID CreateTransactionID(void);
int MMS_MapEncodedHeaders(SBUFFER stream, MMSHEADERS headers);
MMSError MMS_MapEncodedParts(SBUFFER stream, MMSPARTS parts);
MMSError MMS_MapEncodedMessage(GSM_SMSDConfig *Config, SBUFFER Stream, MMSMESSAGE *out);

void MMS_DumpHeaders(SBUFFER buffer, MMSHEADERS headers);
void SaveSBufferToTempFile(GSM_SMSDConfig *Config, SBUFFER Buffer);

void MMS_ContentTypeAsString(SBUFFER buffer, MMSContentType *ct);

MMSError MMS_ParseMediaType(CSTR mime, MMSCONTENTTYPE out);
MMSError MMS_ParseHeaders(MMSHEADERS headers, CSTR headers_string);

#endif //GAMMU_MMS_SERVICE_H
