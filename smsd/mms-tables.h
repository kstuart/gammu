#ifndef GAMMU_MMS_TABLES_H
#define GAMMU_MMS_TABLES_H

#include "mms-data.h"

MMSFIELDINFO WSPFields_FindByID(int id);
MMSFIELDINFO MMSFields_FindByID(int id);
MMSFIELDINFO MMSWellKnownParams_FindByID(int id);
MMSVALUEENUM Charset_FindByID(int id);
MMSVALUEENUM MMS_WkContentType_FindByID(int id);

#endif //GAMMU_MMS_TABLES_H
