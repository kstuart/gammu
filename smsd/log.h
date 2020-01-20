/* (c) 2002-2004 by Marcin Wiacek and Joergen Thomsen */

#ifndef __log_h_
#define __log_h_

#include <gammu-smsd.h>

typedef enum {
	DEBUG_ERROR = -1,
	DEBUG_INFO = 0,
	DEBUG_NOTICE = 1,
	DEBUG_SQL = 2,
	DEBUG_GAMMU = 4,
} SMSD_DebugLevel;

/**
 * Logs a message to SMSD log.
 *
 * \param level Importance level, 0 is an important message,
 *		1,2,4,... are debug information which can be disabled in smsdrc.
 *		-1 is a critical error message
 * \param format printf like format string.
 */
PRINTF_STYLE(3, 4)
void SMSD_Log(int level, GSM_SMSDConfig *Config, const char *format, ...);

/**
 * Logs a message to SMSD log with description of OS specific error code.
 */
void SMSD_LogErrno(GSM_SMSDConfig *Config, const char *message);

#endif

/* How should editor hadle tabs in this file? Add editor commands here.
 * vim: noexpandtab sw=8 ts=8 sts=8:
 */
