/* (c) 2010 by  Miloslav Semler */

#ifndef __sql_h_
#define __sql_h_

#include "../core.h"

extern GSM_SMSDService SMSDSQL;

/**
 * Parses date string into time_t.
 *
 * \return Negative value on failure, -2 for special date "0000-00-00 00:00:00"
 */
time_t SMSDSQL_ParseDate(GSM_SMSDConfig * Config, const char *date);

/**
 * Updates the delivery status of an MMS message
 *
 * @param Config
 * @param msgid the MMS message ID provided by the MMSC
 * @param status the MMS delivery status
 * @param ts delivery timestamp
 */
GSM_Error SMSDSQL_UpdateDeliveryStatusMMS(GSM_SMSDConfig *Config, CSTR msgid, MMSStatus status, time_t ts);
GSM_Error SMSDSQL_SaveReportMMS(GSM_SMSDConfig *Config, GSM_MMSIndicator *MMSIndicator);
#endif

/* How should editor hadle tabs in this file? Add editor commands here.
 * vim: noexpandtab sw=8 ts=8 sts=8:
 */
