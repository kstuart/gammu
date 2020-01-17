#ifndef GAMMU_MMS_SERVICE_H
#define GAMMU_MMS_SERVICE_H

#include <dynamic-buffer.h>
#include "mms-data.h"
#include "mms-tables.h"
#include "mms-decoders.h"


MMSError MMS_DecodeFieldValue(CDBUFFER stream, MMSFIELDINFO fi, MMSValue *out);

int MMS_ParseHeaders(CDBUFFER stream, MMSHEADERS headers);
MMSError MMS_ParseParts(CDBUFFER stream, MMSPARTS parts);
MMSError MMS_ParseMessage(CDBUFFER stream, MMSMESSAGE *out);

void MMS_DumpHeaders(MMSHEADERS headers, DBUFFER buffer);

#endif //GAMMU_MMS_SERVICE_H
