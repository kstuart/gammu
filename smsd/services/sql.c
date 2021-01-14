/**
 * SQL database service
 *
 * Part of Gammu project
 *
 * Copyright (C) 2009 - 2018 Michal Čihař
 *           (c) 2010 Miloslav Semler
 *
 * Licensed under GNU GPL version 2 or later
 */

#define _XOPEN_SOURCE
#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <time.h>
#include <gammu.h>
#include "../../libgammu/misc/strptime.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "../core.h"
#include "../../libgammu/gsmstate.h"
#include "../../libgammu/misc/string.h"
#include "../../libgammu/misc/coding/coding.h"

#include "../streambuffer.h"
#include "../mms-service.h"

#include "../../libgammu/gsmstate.h"
#include "../mms-data.h"

GSM_Error SMSD_FetchMMS(GSM_SMSDConfig *Config, GSM_MMSIndicator *MMSIndicator);

/**
 * Returns name of the SQL dialect to use.
 */
const char *SMSDSQL_SQLName(GSM_SMSDConfig * Config)
{
	if (Config->sql != NULL) {
		return Config->sql;
	} else {
		return Config->driver;
	}
}

/* FIXME: I know this is broken, need to figure out better way */
const char now_plus_odbc[] = "{fn CURRENT_TIMESTAMP()} + INTERVAL %d SECOND";
const char now_plus_mysql[] = "(NOW() + INTERVAL %d SECOND) + 0";
const char now_plus_pgsql[] = "now() + interval '%d seconds'";
const char now_plus_sqlite[] = "datetime('now', '+%d seconds', 'localtime')";
const char now_plus_freetds[] = "DATEADD('second', %d, CURRENT_TIMESTAMP)";
const char now_plus_access[] = "now()+#00:00:%d#";
const char now_plus_oracle[] = "CURRENT_TIMESTAMP + INTERVAL '%d' SECOND";
const char now_plus_fallback[] = "NOW() + INTERVAL %d SECOND";


static const char *SMSDSQL_NowPlus(GSM_SMSDConfig * Config, int seconds)
{
	const char *driver_name;
	static char result[100];

	driver_name = SMSDSQL_SQLName(Config);

	if (strcasecmp(driver_name, "mysql") == 0 || strcasecmp(driver_name, "native_mysql") == 0) {
		sprintf(result, now_plus_mysql, seconds);
	} else if (strcasecmp(driver_name, "pgsql") == 0 || strcasecmp(driver_name, "native_pgsql") == 0) {
		sprintf(result, now_plus_pgsql, seconds);
	} else if (strncasecmp(driver_name, "sqlite", 6) == 0) {
		sprintf(result, now_plus_sqlite, seconds);
	} else if (strcasecmp(driver_name, "freetds") == 0) {
		sprintf(result, now_plus_freetds, seconds);
	} else if (strcasecmp(driver_name, "access") == 0) {
		sprintf(result, now_plus_access, seconds);
	} else if (strcasecmp(driver_name, "oracle") == 0) {
		sprintf(result, now_plus_oracle, seconds);
	} else if (strcasecmp(driver_name, "odbc") == 0) {
		sprintf(result, now_plus_odbc, seconds);
	} else {
		sprintf(result, now_plus_fallback, seconds);
	}
	return result;
}

const char escape_char_odbc[] = "";
const char escape_char_mysql[] = "`";
const char escape_char_pgsql[] = "\"";
const char escape_char_sqlite[] = "";
const char escape_char_freetds[] = "\"";
const char escape_char_fallback[] = "";

static const char *SMSDSQL_EscapeChar(GSM_SMSDConfig * Config)
{
	const char *driver_name;

	driver_name = SMSDSQL_SQLName(Config);

	if (strcasecmp(driver_name, "mysql") == 0 || strcasecmp(driver_name, "native_mysql") == 0) {
		return escape_char_mysql;
	} else if (strcasecmp(driver_name, "pgsql") == 0 || strcasecmp(driver_name, "native_pgsql") == 0) {
		return escape_char_pgsql;
	} else if (strncasecmp(driver_name, "sqlite", 6) == 0) {
		return escape_char_sqlite;
	} else if (strcasecmp(driver_name, "oracle") == 0) {
		return escape_char_fallback;
	} else if (strcasecmp(driver_name, "freetds") == 0 || strcasecmp(driver_name, "mssql") == 0 || strcasecmp(driver_name, "sybase") == 0) {
		return escape_char_freetds;
	} else if (strcasecmp(Config->driver, "odbc") == 0 || strcasecmp(Config->driver, "mssql") == 0) {
		return escape_char_odbc;
	} else {
		return escape_char_fallback;
	}
}

const char rownum_clause_fallback[] = "";


static const char *SMSDSQL_RownumClause(GSM_SMSDConfig * Config, const char *count, gboolean in_where)
{
	const char *driver_name;
	static char result[100];

	driver_name = SMSDSQL_SQLName(Config);

	if (strcasecmp(driver_name, "oracle") == 0 || strcasecmp(driver_name, "freetds") == 0) {
		if (in_where) {
			strcpy(result, " AND ");
		} else {
			strcpy(result, " WHERE ");
		}
		strcat(result, "ROWNUM <= ");
		strcat(result, count);
		strcat(result, " ");
		return result;
	} else {
		return rownum_clause_fallback;
	}
}


const char top_clause_access[] = "TOP";
const char top_clause_fallback[] = "";

static const char *SMSDSQL_TopClause(GSM_SMSDConfig * Config, const char *count)
{
	const char *driver_name;
	static char result[100];

	driver_name = SMSDSQL_SQLName(Config);

	if (strcasecmp(driver_name, "access") == 0 || strcasecmp(driver_name, "mssql") == 0) {
		strcpy(result, top_clause_access);
		strcat(result, " ");
		strcat(result, count);
		strcat(result, " ");
		return result;
	} else {
		return top_clause_fallback;
	}
}

const char limit_clause_access[] = "";
const char limit_clause_fallback[] = "LIMIT";

static const char *SMSDSQL_LimitClause(GSM_SMSDConfig * Config, const char *count)
{
	const char *driver_name;
	static char result[100];

	driver_name = SMSDSQL_SQLName(Config);

	if (strcasecmp(driver_name, "mssql") == 0 || strcasecmp(driver_name, "access") == 0 || strcasecmp(driver_name, "freetds") == 0 || strcasecmp(driver_name, "oracle") == 0) {
		return limit_clause_access;
	} else {
		strcpy(result, limit_clause_fallback);
		strcat(result, " ");
		strcat(result, count);
		return result;
	}
}

const char now_odbc[] = "{fn CURRENT_TIMESTAMP()}";
const char now_mysql[] = "NOW()";
const char now_pgsql[] = "now()";
const char now_sqlite[] = "datetime('now', 'localtime')";
const char now_freetds[] = "CURRENT_TIMESTAMP";
const char now_access[] = "now()";
const char now_fallback[] = "NOW()";

const char currtime_odbc[] = "{fn CURTIME()}";
const char currtime_mysql[] = "CURTIME()";
const char currtime_pgsql[] = "localtime";
const char currtime_sqlite[] = "time('now', 'localtime')";
const char currtime_freetds[] = "CURRENT_TIME";
const char currtime_fallback[] = "CURTIME()";

static const char *SMSDSQL_CurrentTime(GSM_SMSDConfig * Config)
{
	const char *driver_name;

	driver_name = SMSDSQL_SQLName(Config);

	if (strcasecmp(driver_name, "mysql") == 0 || strcasecmp(driver_name, "native_mysql") == 0) {
		return currtime_mysql;
	} else if (strcasecmp(driver_name, "pgsql") == 0 || strcasecmp(driver_name, "native_pgsql") == 0) {
		return currtime_pgsql;
	} else if (strncasecmp(driver_name, "sqlite", 6) == 0) {
		return currtime_sqlite;
	} else if (strcasecmp(Config->driver, "oracle") == 0 || strcasecmp(driver_name, "freetds") == 0 || strcasecmp(driver_name, "mssql") == 0 || strcasecmp(driver_name, "sybase") == 0) {
		return currtime_freetds;
	} else if (strcasecmp(Config->driver, "odbc") == 0) {
		return currtime_odbc;
	} else {
		return currtime_fallback;
	}
}
static const char *SMSDSQL_Now(GSM_SMSDConfig * Config)
{
	const char *driver_name;

	driver_name = SMSDSQL_SQLName(Config);

	if (strcasecmp(driver_name, "mysql") == 0 || strcasecmp(driver_name, "native_mysql") == 0) {
		return now_mysql;
	} else if (strcasecmp(driver_name, "pgsql") == 0 || strcasecmp(driver_name, "native_pgsql") == 0) {
		return now_pgsql;
	} else if (strncasecmp(driver_name, "sqlite", 6) == 0) {
		return now_sqlite;
	} else if (strcasecmp(driver_name, "oracle") == 0 || strcasecmp(driver_name, "freetds") == 0 || strcasecmp(driver_name, "mssql") == 0 || strcasecmp(driver_name, "sybase") == 0) {
		return now_freetds;
	} else if (strcasecmp(driver_name, "access") == 0) {
		return now_access;
	} else if (strcasecmp(Config->driver, "odbc") == 0) {
		return now_odbc;
	} else {
		return now_fallback;
	}
}

static GSM_Error SMSDSQL_Reconnect(GSM_SMSDConfig * Config)
{
	GSM_Error error = ERR_DB_TIMEOUT;
	int attempts;
	struct GSM_SMSDdbobj *db = Config->db;

	SMSD_Log(DEBUG_INFO, Config, "Reconnecting to the database!");
	for (attempts = 1; attempts <= Config->backend_retries; attempts++) {
		SMSD_Log(DEBUG_INFO, Config, "Reconnecting after %d seconds...", attempts * attempts);
		sleep(attempts * attempts);
		db->Free(Config);
		error = db->Connect(Config);
		if (error == ERR_NONE) {
			return ERR_NONE;
		}
	}
	return error;
}

static GSM_Error SMSDSQL_Query(GSM_SMSDConfig * Config, const char *query, SQL_result * res)
{
	GSM_Error error = ERR_DB_TIMEOUT;
	int attempts;
	struct GSM_SMSDdbobj *db = Config->db;

	for (attempts = 1; attempts <= Config->backend_retries; attempts++) {
		SMSD_Log(DEBUG_SQL, Config, "Execute SQL: %s", query);
		error = db->Query(Config, query, res);
		if (error == ERR_NONE) {
			return ERR_NONE;
		}

		if (error != ERR_DB_TIMEOUT){
			SMSD_Log(DEBUG_INFO, Config, "SQL failure: %d", error);
			return error;
		}

		SMSD_Log(DEBUG_INFO, Config, "SQL failed (timeout): %s", query);
		/* We will try to reconnect */
		error = SMSDSQL_Reconnect(Config);
		if (error != ERR_NONE) {
			return ERR_DB_TIMEOUT;
		}
	}
	return error;
}

/*
 * generates a timestamp string suitable for inserting into a database, the timestamp
 * argument must be a valid POSIX calendar time.
 *
 * pgsql: https://www.postgresql.org/docs/9.1/datatype-datetime.html
 *   - "For timestamp with time zone, the internally stored value is always in UTC
 *      (Universal Coordinated Time, traditionally known as Greenwich Mean Time, GMT).
 *      An input value that has an explicit time zone specified is converted to UTC
 *      using the appropriate offset for that time zone.
 *
 *      If no time zone is stated in the input string, then it is assumed to be in the
 *      time zone indicated by the system's timezone parameter, and is converted to
 *      UTC using the offset for the timezone zone. When a timestamp with time zone
 *      value is output, it is always converted from UTC to the current timezone zone,
 *      and displayed as local time in that zone. To see the time in another time zone,
 *      either change timezone or use the AT TIME ZONE construct (see Section 9.9.3).
 *
 *      Conversions between timestamp without time zone and timestamp with time zone
 *      normally assume that the timestamp without time zone value should be taken or
 *      given as timezone local time."
 *
 * mysql: https://dev.mysql.com/doc/refman/8.0/en/datetime.html
 *  - "MySQL converts TIMESTAMP values from the current time zone to UTC for
 *     storage, and back from UTC to the current time zone for retrieval."
 *
 * oracle: https://docs.oracle.com/cd/B19306_01/server.102/b14225/ch4datetime.htm#i1006050
 *  - a TIMESTAMP literal without tz info is interpreted as local time zone.
 */
void SMSDSQL_Time2String(GSM_SMSDConfig * Config, time_t timestamp, char *static_buff, size_t size)
{
	const char *driver_name = SMSDSQL_SQLName(Config);
	struct tm *tm = localtime(&timestamp);

  if(timestamp == -2) {
    strcpy(static_buff, "0000-00-00 00:00:00");
  }
  else if (strcasecmp(driver_name, "oracle") == 0) {
    strftime(static_buff, size, "TIMESTAMP '%Y-%m-%d %H:%M:%S'", tm);
  }
  else if (strcasecmp(Config->driver, "odbc") == 0) {
    strftime(static_buff, size, "{ ts '%Y-%m-%d %H:%M:%S' }", tm);
  }
  else if (strcasecmp(driver_name, "access") == 0) {
    strftime(static_buff, size, "'%Y-%m-%d %H:%M:%S'", tm);
  }
  else {
    strftime(static_buff, size, "%Y-%m-%d %H:%M:%S", tm);
  }
}

static GSM_Error SMSDSQL_NamedQuery(GSM_SMSDConfig * Config, const char *sql_query, GSM_SMSMessage *sms,
	GSM_MultiSMSMessage * smsmulti, const SQL_Var *params, SQL_result * res, gboolean retry)
{
	char buff[65536], *ptr, c, static_buff[8192];
	char *buffer2, *end;
	const char *to_print, *q = sql_query;
	int int_to_print;
	int numeric;
	int n, argc = 0, i;
	struct GSM_SMSDdbobj *db = Config->db;
	GSM_MultiPartSMSInfo SMSInfo;

	if (params != NULL) {
		while (params[argc].type != SQL_TYPE_NONE) argc++;
	}

	ptr = buff;

	do {
		if (*q != '%') {
			*ptr++ = *q;
			continue;
		}
		c = *(++q);
		if( c >= '0' && c <= '9'){
			n = strtoul(q, &end, 10) - 1;
			if (n < argc && n >= 0) {
				switch(params[n].type){
					case SQL_TYPE_INT:
						ptr += sprintf(ptr, "%lli", params[n].v.i);
						break;
					case SQL_TYPE_STRING:
						buffer2 = db->QuoteString(Config, params[n].v.s);
						memcpy(ptr, buffer2, strlen(buffer2));
						ptr += strlen(buffer2);
						free(buffer2);
						break;
					default:
						SMSD_Log(DEBUG_ERROR, Config, "SQL: unknown type: %i (application bug) in query: `%s`", params[n].type, sql_query);
						return ERR_BUG;
						break;
				}
			} else {
				SMSD_Log(DEBUG_ERROR, Config, "SQL: wrong number of parameter: %i (max %i) in query: `%s`", n+1, argc, sql_query);
				return ERR_BUG;
			}
			q = end - 1;
			continue;
		}
		numeric = 0;
		to_print = NULL;
		switch (c) {
			case 'I':
				to_print = Config->Status->IMEI;
				break;
			case 'S':
				to_print = Config->Status->IMSI;
				break;
			case 'P':
				to_print = Config->PhoneID;
				break;
			case 'O':
				to_print = Config->Status->NetInfo.NetworkCode;
				break;
			case 'M':
				to_print = DecodeUnicodeConsole(Config->Status->NetInfo.NetworkName);
				break;
			case 'N':
				snprintf(static_buff, sizeof(static_buff), "Gammu %s, %s, %s", GAMMU_VERSION, GetOS(), GetCompiler());
				to_print = static_buff;
				break;
			case 'A':
				to_print = Config->CreatorID;
				break;
			default:
				if (sms != NULL) {
					switch (c) {
						case 'R':
							/*
							 * Always store international numnbers with + prefix
							 * to allow easy matching later.
							 */
							if (sms->Number[0] == '0' && sms->Number[1] == '0') {
								static_buff[0] = '+';
								EncodeUTF8(static_buff + 1, sms->Number + 2);
							} else {
								EncodeUTF8(static_buff, sms->Number);
							}
							to_print = static_buff;
							break;
						case 'F':
							EncodeUTF8(static_buff, sms->SMSC.Number);
							to_print = static_buff;
							break;
						case 'u':
							if (sms->UDH.Type != UDH_NoUDH) {
								EncodeHexBin(static_buff, sms->UDH.Text, sms->UDH.Length);
								to_print = static_buff;
							}else{
								to_print = "";
							}
							break;
						case 'x':
							int_to_print =  sms->Class;
							numeric = 1;
							break;
						case 'c':
							to_print = GSM_SMSCodingToString(sms->Coding);
							break;
						case 't':
							int_to_print =  sms->MessageReference;
							numeric = 1;
							break;
						case 'E':
							switch (sms->Coding) {
								case SMS_Coding_Unicode_No_Compression:
								case SMS_Coding_Default_No_Compression:
								case SMS_Coding_ASCII:
									EncodeHexUnicode(static_buff, sms->Text, UnicodeLength(sms->Text));
									break;
								case SMS_Coding_8bit:
									EncodeHexBin(static_buff, sms->Text, sms->Length);
									break;
								default:
									*static_buff = '\0';
									break;
							}
							to_print = static_buff;
							break;
						case 'T':
							/*
							 * Print empty string on retry as the error is quite likely in
							 * corrupted text.
							 */
							if (retry) {
								to_print = "";
							} else {
								if (smsmulti != NULL) {
									if (!smsmulti->Processed && sms == &smsmulti->SMS[0]) {
										static_buff[0] = 0;
										if (GSM_DecodeMultiPartSMS(GSM_GetDebug(Config->gsm), &SMSInfo, smsmulti, TRUE)) {
											for (i = 0; i < SMSInfo.EntriesNum; i++) {
												switch (SMSInfo.Entries[i].ID) {
													case SMS_ConcatenatedTextLong:
													case SMS_ConcatenatedAutoTextLong:
													case SMS_ConcatenatedTextLong16bit:
													case SMS_ConcatenatedAutoTextLong16bit:
														EncodeUTF8(static_buff + strlen(static_buff), SMSInfo.Entries[i].Buffer);
														break;
													default:
														break;
												}
											}
										}
										GSM_FreeMultiPartSMSInfo(&SMSInfo);
										if (static_buff[0] != 0) {
											to_print = static_buff;
											smsmulti->Processed = TRUE;
										}
									} else if (smsmulti->Processed) {
										to_print = "";
									}
								}
								if (to_print == NULL) {
									if(sms->Class == GSM_CLASS_MMS) {
										EncodeUTF8(static_buff, Config->MMSIndicatorMsg);
										to_print = static_buff;
									}
									else {
										switch (sms->Coding) {
											case SMS_Coding_Unicode_No_Compression:
											case SMS_Coding_Default_No_Compression:
											case SMS_Coding_ASCII:
											case SMS_Coding_8bit:
												EncodeUTF8(static_buff, sms->Text);
												to_print = static_buff;
												break;
											default:
												to_print = "";
												break;
										}
									}
								}
							}
							break;
						case 'V':
							if (sms->SMSC.Validity.Format == SMS_Validity_RelativeFormat) {
								int_to_print = sms->SMSC.Validity.Relative;
							} else {
								int_to_print =  -1;
							}
							numeric = 1;
							break;
						case 'C':
							SMSDSQL_Time2String(Config, Fill_Time_T(sms->SMSCTime), static_buff, sizeof(static_buff));
							to_print = static_buff;
							break;
						case 'd':
							SMSDSQL_Time2String(Config, Fill_Time_T(sms->DateTime), static_buff, sizeof(static_buff));
							to_print = static_buff;
							break;
						case 'e':
							int_to_print = sms->DeliveryStatus;
							numeric = 1;
							break;
						default:
							SMSD_Log(DEBUG_ERROR, Config, "SQL: uexpected char '%c' in query: %s", c, sql_query);
							return ERR_BUG;

					} /* end of switch */
				} else {
					SMSD_Log(DEBUG_ERROR, Config, "Syntax error in query.. uexpected char '%c' in query: %s", c, sql_query);
					return ERR_BUG;
				}
				break;
		} /* end of switch */
		if (numeric) {
			ptr += sprintf(ptr, "%i", int_to_print);
		} else if (to_print != NULL) {
			buffer2 = db->QuoteString(Config, to_print);
			memcpy(ptr, buffer2, strlen(buffer2));
			ptr += strlen(buffer2);
			free(buffer2);
		} else {
			memcpy(ptr, "NULL", 4);
			ptr += 4;
		}
	} while (*(++q) != '\0');
	*ptr = '\0';
	return SMSDSQL_Query(Config, buff, res);

}

static GSM_Error SMSDSQL_CheckTable(GSM_SMSDConfig * Config, const char *table)
{
	SQL_result res;
	char buffer[200];
	GSM_Error error;
	struct GSM_SMSDdbobj *db = Config->db;

	sprintf(
		buffer,
		"SELECT %s * FROM %s %s %s",
		SMSDSQL_TopClause(Config, "1"),
		table,
		SMSDSQL_RownumClause(Config, "1", FALSE),
		SMSDSQL_LimitClause(Config, "1")
	);
	error = SMSDSQL_Query(Config, buffer, &res);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_ERROR, Config, "Table %s not found, disconnecting!", table);
		db->Free(Config);
		return error;
	}
	db->FreeResult(Config, &res);
	return ERR_NONE;
}

/* Disconnects from a database */
static GSM_Error SMSDSQL_Free(GSM_SMSDConfig * Config)
{
	int i;
	SMSD_Log(DEBUG_SQL, Config, "Disconnecting from SQL database.");
	Config->db->Free(Config);
	/* free configuration */
	for(i = 0; i < SQL_QUERY_LAST_NO; i++){
		free(Config->SMSDSQL_queries[i]);
		Config->SMSDSQL_queries[i] = NULL;
	}
	return ERR_NONE;
}

/* Connects to database */
static GSM_Error SMSDSQL_Init(GSM_SMSDConfig * Config)
{
	SQL_result res;
	int version;
	GSM_Error error;
	struct GSM_SMSDdbobj *db;
	const char *escape_char;
	char buffer[100];

#ifdef WIN32
	_tzset();
#else
	tzset();
#endif

	db = Config->db;

	error = db->Connect(Config);
	if (error != ERR_NONE)
		return error;

	error = SMSDSQL_CheckTable(Config, Config->table_gammu);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to open table %s", Config->table_gammu);
		return error;
	}

	error = SMSDSQL_CheckTable(Config, Config->table_inbox);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to open table %s", Config->table_inbox);
		return error;
	}

	error = SMSDSQL_CheckTable(Config, Config->table_sentitems);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to open table %s", Config->table_sentitems);
		return error;
	}

	error = SMSDSQL_CheckTable(Config, Config->table_outbox);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to open table %s", Config->table_outbox);
		return error;
	}

	error = SMSDSQL_CheckTable(Config, Config->table_outbox_multipart);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to open table %s", Config->table_outbox_multipart);
		return error;
	}

	error = SMSDSQL_CheckTable(Config, Config->table_phones);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to open table %s", Config->table_phones);
		return error;
	}

	escape_char = SMSDSQL_EscapeChar(Config);

	sprintf(buffer, "SELECT %sVersion%s FROM %s", escape_char, escape_char, Config->table_gammu);
	error = SMSDSQL_Query(Config, buffer, &res);
	if (error != ERR_NONE) {
		db->Free(Config);
		return error;
	}
	if (db->NextRow(Config, &res) != 1) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to seek to first row!");
		db->FreeResult(Config, &res);
		db->Free(Config);
		return ERR_UNKNOWN;
	}
	version = (int)db->GetNumber(Config, &res, 0);
	db->FreeResult(Config, &res);
	error = SMSD_CheckDBVersion(Config, version);
	if (error != ERR_NONE) {
		db->Free(Config);
		return error;
	}

	SMSD_Log(DEBUG_INFO, Config, "Connected to Database %s: %s on %s", Config->driver, Config->database, Config->host);

	return ERR_NONE;
}

static GSM_Error SMSDSQL_InitAfterConnect(GSM_SMSDConfig * Config)
{
	SQL_result res;
	GSM_Error error;
	struct GSM_SMSDdbobj *db = Config->db;
	SQL_Var vars[3] = {{SQL_TYPE_STRING, {NULL}}, {SQL_TYPE_STRING, {NULL}}, {SQL_TYPE_NONE, {NULL}}};

	error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_DELETE_PHONE], NULL, NULL, NULL, &res, FALSE);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_INFO, Config, "Error deleting from database (%s)", __FUNCTION__);
		return error;
	}
	db->FreeResult(Config, &res);

	SMSD_Log(DEBUG_INFO, Config, "Inserting phone info");
	vars[0].v.s = Config->enable_send ? "yes" : "no";
	vars[1].v.s = Config->enable_receive ? "yes" : "no";

	error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_INSERT_PHONE], NULL, NULL, vars, &res, FALSE);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_INFO, Config, "Error inserting into database (%s)", __FUNCTION__);
		return error;
	}
	db->FreeResult(Config, &res);

	return ERR_NONE;
}

GSM_Error SaveMMSParts(GSM_SMSDConfig *Config, unsigned long long inbox_id, MMSMESSAGE mms, SBUFFER buffer)
{
	GSM_Error error = ERR_NONE;
	SQL_result result;


	for(size_t i = 0; i < mms->Parts->end; i++) {
		MMSPART p = &mms->Parts->entries[i];
		MMSHEADER h = MMSHeader_FindByID(p->headers, MMS_HEADER, MMS_CONTENT_TYPE);
		if(!h) {
			SMSD_Log(DEBUG_ERROR, Config, "Failed to find content type for MMS part.");
			return ERR_ABORTED;
		}

		SB_Clear(buffer);
		SB_PutFormattedString(
			buffer,
			"insert into inbox_mms_parts (\"INBOX_ID\", \"MediaType\", \"Data\") values (%d, '",
			inbox_id);
		MMS_ContentTypeAsString(buffer, &h->value.v.content_type);
		SB_PutString(buffer,"', decode('");
		SB_PutAsEncodedBase64(buffer, p->data, p->data_len);
		SB_PutString(buffer, "', 'base64'));");
		SB_PutByte(buffer, 0);
		error = SMSDSQL_Query(Config, SBBase(buffer), &result);

		Config->db->FreeResult(Config, &result);
		if(error != ERR_NONE) {
			SMSD_Log(DEBUG_ERROR, Config, "Error saving MMS part.");
			return error;
		}
	}

	return  error;
}

GSM_Error SMSDSQL_UpdateDeliveryStatusMMS(GSM_SMSDConfig *Config, CSTR msgid, MMSStatus status, time_t ts)
{
	GSM_Error error;
	SQL_result result;
	struct GSM_SMSDdbobj *db = Config->db;
	SBUFFER buf = SB_InitWithCapacity(1024);
	CSTR StatusValue;
	char deliveryTime[60];

	assert(Config);
	assert(msgid);

	switch(status) {
		default: StatusValue = "Error";
		break;
		case MMS_STATUS_EXPIRED:
		case MMS_STATUS_REJECTED: StatusValue = "DeliveryFailed"; break;
		case MMS_STATUS_RETRIEVED: StatusValue = "DeliveryOK"; break;
		case MMS_STATUS_DEFERRED: StatusValue = "DeliveryPending"; break;
	}

	SMSDSQL_Time2String(Config, ts, deliveryTime, 60);

	SB_PutFormattedString(buf, "update sentitems set \"Status\" = '%s', \"DeliveryDateTime\" = '%s' where \"MMS_ID\" LIKE '%s%%';",
		StatusValue,
		deliveryTime,
		msgid);
	SB_PutByte(buf, 0);

	error = SMSDSQL_Query(Config, SBBase(buf), &result);
	if(error != ERR_NONE) {
		SB_Destroy(&buf);
		return error;
	}

	SB_Destroy(&buf);

	if (db->AffectedRows(Config, &result) == 0) {
		SMSD_Log(DEBUG_INFO, Config, "Received MMS delivery report but the Message-ID(%s) is not recognized.", msgid);
	}

	db->FreeResult(Config, &result);

	return ERR_NONE;
}

GSM_Error SMSDSQL_SaveReportMMS(GSM_SMSDConfig *Config, GSM_MMSIndicator *MMSIndicator)
{
	GSM_Error error;
	SQL_result result;
	char deliveryTime[60];
	SBUFFER buf = SB_InitWithCapacity(1024);

	SB_PutFormattedString(buf, "select \"ID\", \"Status\", \"MMSReports\" from sentitems where \"MMS_ID\" = '%s';",
		MMSIndicator->MessageID);
	SB_PutByte(buf, 0);

	error = SMSDSQL_Query(Config, SBBase(buf), &result);
	if(error != ERR_NONE) {
		SB_Destroy(&buf);
		return error;
	}

	if(!Config->db->NextRow(Config, &result)) {
		SMSD_Log(DEBUG_INFO, Config, "Received MMS delivery report but the Message-ID(%s) is not recognized.",
			MMSIndicator->MessageID);
		Config->db->FreeResult(Config, &result);
		SB_Destroy(&buf);
		return ERR_INVALIDDATA;
	}

	CSTR sent_id = Config->db->GetString(Config, &result, 0);
	CSTR old_status = Config->db->GetString(Config, &result, 1);
	CSTR reports = Config->db->GetString(Config, &result, 2);
	int hasDeliveryFailures = strcmp(old_status, "DeliveryFailed") == 0;

	MMSVALUEENUM status = MMS_StatusValue_FindByID(MMSIndicator->Status);
	SMSDSQL_Time2String(Config, MMSIndicator->Date, deliveryTime, 60);

	SB_Clear(buf);
	SB_PutFormattedString(buf, "update sentitems set \"MMSReports\" = '%sTo %s On %s Status: %s\n' where \"ID\" = '%s';",
	  reports,
	  MMSIndicator->Recipient,
	  deliveryTime,
	  status ? status->name : "<unknown>",
	  sent_id);
	SB_PutByte(buf, 0);
	Config->db->FreeResult(Config, &result);

	error = SMSDSQL_Query(Config, SBBase(buf), &result);
	if(error != ERR_NONE)
		SMSD_Log(DEBUG_ERROR, Config, "Failed to update DB with delivery report");

	Config->db->FreeResult(Config, &result);
	SB_Destroy(&buf);

	if(error == ERR_NONE)
		error = SMSDSQL_UpdateDeliveryStatusMMS(Config, MMSIndicator->MessageID,
			 hasDeliveryFailures ? MMS_STATUS_REJECTED : MMSIndicator->Status,
			 MMSIndicator->Date);

	return error;
}

GSM_Error SaveInboxMMS(GSM_SMSDConfig *Config, MMSMESSAGE mms, unsigned long long inbox_id)
{
	GSM_Error error;
	SQL_result result;
	SBUFFER buf = SB_InitWithCapacity(20480);
	char *sender = NULL;

	MMSHEADER from = MMSHeader_FindByID(mms->Headers, MMS_HEADER, MMS_FROM);
	if(!from) {
		SMSD_Log(DEBUG_INFO, Config, "MMS Headers does not include From field.");
	}
	else {
		sender = strndup(from->value.v.str,
			 strchr(from->value.v.str, '/') - from->value.v.str);
	}

	SMSD_Log(DEBUG_INFO, Config, "InboxID(%llu)", inbox_id);
	SB_PutFormattedString(buf, "update inbox set \"Class\" = %d, \"Status\" = %d, \"SenderNumber\" = '%s' where \"ID\" = %llu;",
		GSM_CLASS_MMS,
		Config->StatusCode,
		sender ? sender : "Not Provided",
		inbox_id);
	SB_PutByte(buf, 0);
	free(sender);

	error = SMSDSQL_Query(Config, SBBase(buf), &result);
	if(error != ERR_NONE) {
		SB_Destroy(&buf);
		return error;
	}
	Config->db->FreeResult(Config, &result);

	if(Config->StatusCode >= 400) {
		SB_Destroy(&buf);
		return ERR_NONE;
	}

	SB_Clear(buf);
	SB_PutString(buf, "update inbox set \"MMSHeaders\" = '");
	MMS_DumpHeaders(buf, mms->Headers);
	SB_PutFormattedString(buf, "' where \"ID\" = %llu;", inbox_id);
	SB_PutByte(buf, 0);
	error = SMSDSQL_Query(Config, SBBase(buf), &result);
	if(error != ERR_NONE) {
		SB_Destroy(&buf);
        return error;
	}
	Config->db->FreeResult(Config, &result);

	error = SaveMMSParts(Config, inbox_id, mms, buf);
	SB_Destroy(&buf);
	SB_Clear(Config->MMSBuffer);

	return error;
}

GSM_Error MMS_ProcessMMSIndicator(GSM_SMSDConfig *Config, unsigned long long inbox_id, GSM_MMSIndicator *MMSIndicator);

/* Save SMS from phone (called Inbox sms - it's in phone Inbox) somewhere */
static GSM_Error SMSDSQL_SaveInboxSMS(GSM_MultiSMSMessage * sms, GSM_SMSDConfig * Config, char **Locations)
{
	SQL_result res, res2;
	SQL_Var vars[3];
	GSM_Error error;
	struct GSM_SMSDdbobj *db = Config->db;
	const char *q, *status;

	char smstext[3 * GSM_MAX_SMS_LENGTH + 1];
	char destinationnumber[3 * GSM_MAX_NUMBER_LENGTH + 1];
	char smsc_message[3 * GSM_MAX_NUMBER_LENGTH + 1];
	int i;
	time_t t_time1, t_time2;
	gboolean found;
	long diff;
	unsigned long long new_id;
	size_t locations_size = 0, locations_pos = 0;
	const char *state, *smsc;
	int idx_last = sms->Number - 1;

	*Locations = NULL;
	sms->Processed = FALSE;

	for (i = 0; i < sms->Number; i++) {
		EncodeUTF8(destinationnumber, sms->SMS[i].Number);
		EncodeUTF8(smsc_message, sms->SMS[i].SMSC.Number);
		if (sms->SMS[i].PDU == SMS_Status_Report) {
			EncodeUTF8(smstext, sms->SMS[i].Text);
			SMSD_Log(DEBUG_INFO, Config, "Delivery report: %s to %s", smstext, destinationnumber);

			error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_SAVE_INBOX_SMS_SELECT], &sms->SMS[i], sms, NULL, &res, FALSE);
			if (error != ERR_NONE) {
				SMSD_Log(DEBUG_INFO, Config, "Error reading from database (%s)", __FUNCTION__);
				return error;
			}

			found = FALSE;
			while (db->NextRow(Config, &res)) {
				state = db->GetString(Config, &res, 1);
				smsc = db->GetString(Config, &res, 4);
				SMSD_Log(DEBUG_NOTICE, Config, "Checking for delivery report, SMSC=%s, state=%s", smsc, state);

				if (strcmp(smsc, smsc_message) != 0) {
					if (Config->skipsmscnumber[0] == 0 || strcmp(Config->skipsmscnumber, smsc)) {
						SMSD_Log(DEBUG_ERROR, Config, "Failed to match SMSC, you might want to use SkipSMSCNumber (sent: %s, received: %s)", smsc_message, smsc);
						continue;
					}
				}

				if (strcmp(state, "SendingOK") == 0 || strcmp(state, "DeliveryPending") == 0) {
					t_time1 = db->GetDate(Config, &res, 2);
					if (t_time1 < 0) {
						SMSD_Log(DEBUG_ERROR, Config, "Invalid SendingDateTime -1 for SMS TPMR=%i", sms->SMS[i].MessageReference);
						return ERR_UNKNOWN;
					}
					t_time2 = Fill_Time_T(sms->SMS[i].DateTime);
					diff = t_time2 - t_time1;

					if (diff > -Config->deliveryreportdelay && diff < Config->deliveryreportdelay) {
						found = TRUE;
						break;
					} else {
						SMSD_Log(DEBUG_NOTICE, Config,
							 "Delivery report would match, but time delta is too big (%ld), consider increasing DeliveryReportDelay", diff);
					}
				}
			}

			if (found) {
				if (!strcmp(smstext, "Delivered")) {
					q = Config->SMSDSQL_queries[SQL_QUERY_SAVE_INBOX_SMS_UPDATE_DELIVERED];
				} else {
					q = Config->SMSDSQL_queries[SQL_QUERY_SAVE_INBOX_SMS_UPDATE];
				}

				if (!strcmp(smstext, "Delivered")) {
					status = "DeliveryOK";
				} else if (!strcmp(smstext, "Failed")) {
					status = "DeliveryFailed";
				} else if (!strcmp(smstext, "Pending")) {
					status = "DeliveryPending";
				} else if (!strcmp(smstext, "Unknown")) {
					status = "DeliveryUnknown";
				} else {
					status = "";
				}

				vars[0].type = SQL_TYPE_STRING;
				vars[0].v.s = status;			/* Status */
				vars[1].type = SQL_TYPE_INT;
				vars[1].v.i = (long)db->GetNumber(Config, &res, 0); /* ID */
				vars[2].type = SQL_TYPE_NONE;

				error = SMSDSQL_NamedQuery(Config, q, &sms->SMS[i], sms, vars, &res2, FALSE);
				if (error != ERR_NONE) {
					SMSD_Log(DEBUG_INFO, Config, "Error writing to database (%s)", __FUNCTION__);
					return error;
				}
				db->FreeResult(Config, &res2);
			} else {
				SMSD_Log(DEBUG_ERROR, Config, "Failed to find SMS for TPMR=%i, Number=%s", sms->SMS[i].MessageReference, sms->SMS[i].Number);
			}
			db->FreeResult(Config, &res);
			continue;
		}

		if (sms->SMS[i].PDU != SMS_Deliver)
			continue;

		if(i == idx_last &&
			 sms->SMS[i].UDH.Type == UDH_UserUDH &&
			 *(unsigned short*)&sms->SMS[i].UDH.Text[3] == 0x840b)
		{
		  sms->SMS[i].Class = GSM_CLASS_MMS;
		}

		error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_SAVE_INBOX_SMS_INSERT], &sms->SMS[i], sms, NULL, &res, FALSE);
		if (error != ERR_NONE) {
			if (error != ERR_DB_TIMEOUT) {
				error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_SAVE_INBOX_SMS_INSERT], &sms->SMS[i], sms, NULL, &res, TRUE);
			}
			if (error != ERR_NONE) {
				SMSD_Log(DEBUG_INFO, Config, "Error writing to database (%s)", __FUNCTION__);
				return error;
			}
		}

		new_id = db->SeqID(Config, "inbox_ID_seq");
		if (new_id == 0) {
			SMSD_Log(DEBUG_INFO, Config, "Failed to get inserted row ID (%s)", __FUNCTION__);
			return ERR_UNKNOWN;
		}
		SMSD_Log(DEBUG_NOTICE, Config, "Inserted message id %lu", (long)new_id);

		if (i == idx_last) {
			GSM_MultiPartSMSInfo SMSInfo;
			if (GSM_DecodeMultiPartSMS(GSM_GetDebug(Config->gsm), &SMSInfo, sms, TRUE)) {
				for (int n = 0; n < SMSInfo.EntriesNum; n++) {
					if (SMSInfo.Entries[n].ID == SMS_MMSIndicatorLong) {
						if(Config->MMSAutoDownload) {
							error = MMS_ProcessMMSIndicator(Config, new_id, SMSInfo.Entries[0].MMSIndicator);
						}
						else {
							SMSD_Log(DEBUG_INFO, Config, "Not configured to download MMS messages, skipping.");
						}
					}
				}
				GSM_FreeMultiPartSMSInfo(&SMSInfo);
			}
		}

		db->FreeResult(Config, &res);

		if (new_id != 0) {
			if (locations_pos + 10 >= locations_size) {
				locations_size += 40;
				*Locations = (char *)realloc(*Locations, locations_size);
				assert(*Locations != NULL);
				if (locations_pos == 0) {
					*Locations[0] = 0;
				}
			}
			locations_pos += sprintf((*Locations) + locations_pos, "%lu ", (long)new_id);
		}

		error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_UPDATE_RECEIVED], &sms->SMS[i], sms, NULL, &res2, FALSE);
		if (error != ERR_NONE) {
			SMSD_Log(DEBUG_INFO, Config, "Error updating number of received messages (%s)", __FUNCTION__);
			return error;
		}
		db->FreeResult(Config, &res2);

	}

	return ERR_NONE;
}

static GSM_Error SMSDSQL_RefreshSendStatus(GSM_SMSDConfig * Config, char *ID)
{
	SQL_result res;
	struct GSM_SMSDdbobj *db = Config->db;
	GSM_Error error;
	SQL_Var vars[2] = {
		{SQL_TYPE_STRING, {ID}},
		{SQL_TYPE_NONE, {NULL}}};

	error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_REFRESH_SEND_STATUS], NULL, NULL, vars, &res, FALSE);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_INFO, Config, "Error writing to database (%s)", __FUNCTION__);
		return error;
	}

	if (db->AffectedRows(Config, &res) == 0) {
		db->FreeResult(Config, &res);
		return ERR_UNKNOWN;
	}

	db->FreeResult(Config, &res);
	return ERR_NONE;
}

static GSM_Error SMSDSQL_UpdateRetries(GSM_SMSDConfig * Config, char *ID)
{
	SQL_result res;
	GSM_Error error;
	size_t query_type;
	struct GSM_SMSDdbobj *db = Config->db;
	/* 1 = ID, 2 = Retries, 3 = StatusCode, 4 = SequencePosition */
	SQL_Var vars[5] = {
		{SQL_TYPE_STRING, {ID}},
		{SQL_TYPE_INT, {NULL}},
		{SQL_TYPE_INT, {NULL}},
		{SQL_TYPE_INT, {NULL}},
		{SQL_TYPE_NONE, {NULL}}};
	vars[1].v.i = Config->retries;
	vars[2].v.i = Config->StatusCode;
	vars[3].v.i = Config->Part;

	error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_UPDATE_RETRIES], NULL, NULL, vars, &res, FALSE);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_INFO, Config, "Error writing to database (%s)", __FUNCTION__);
		return error;
	}

	if (db->AffectedRows(Config, &res) == 0) {
		db->FreeResult(Config, &res);
		return ERR_UNKNOWN;
	}

	db->FreeResult(Config, &res);

	if (Config->StatusCode != -1) {
		query_type = (Config->Part == 1) ? SQL_QUERY_UPDATE_OUTBOX_STATUSCODE : SQL_QUERY_UPDATE_OUTBOX_MULTIPART_STATUSCODE;
		error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[query_type], NULL, NULL, vars, &res, FALSE);
		if (error != ERR_NONE) {
			SMSD_Log(DEBUG_INFO, Config, "Error updating StatusCode (%s)", __FUNCTION__);
			return error;
		}
		db->FreeResult(Config, &res);
	}

	return ERR_NONE;
}



GSM_Error SMSDSQL_PrepareOutboxMMS(GSM_SMSDConfig *Config, long outbox_id, const char *destination, const char *headers,
                                   SBUFFER MMSBuffer)
{
	GSM_Error error;
	SQL_result res;
	SBUFFER buf = SB_InitWithCapacity(4096);
	const char * from = Config->gsm->CurrentConfig->PhoneNumber;
	struct GSM_SMSDdbobj *db = Config->db;
	LocalTXID txid = MMS_CreateTransactionID();

	SB_PutFormattedString(buf,
	                      "select \"MediaType\", \"DataLength\", \"Data\", \"End\" "
											 "from outbox_mms_parts where \"OUTBOX_ID\" = %ld order by \"ID\";", outbox_id);

	MMSMESSAGE m = MMSMessage_Init();
	if(!m) {
		SMSD_Log(DEBUG_ERROR, Config, "Error allocating MMS message");
		return ERR_MEMORY;
	}

	MMSMessage_SetMessageType(m, M_SEND_REQ);
	MMSMessage_SetTransactionID(m, txid);
	MMSMessage_SetMessageVersion(m, MMS_VERSION_12);

	if(Config->currdeliveryreport == 1)
		MMSMessage_SetDeliveryReport(m, MMS_YESNO_YES);

	EncodedString es;
	es.charset = MMS_Charset_FindByID(CHARSET_ASCII);
	es.text = (STR)from;
	MMSMessage_CopyAddressFrom(m, &es);

	es.text = (STR)destination;
	MMSMessage_CopyAddressTo(m, &es);

	MMS_ParseHeaders(m->Headers, headers);

	time_t timeout = time(NULL) + 5;
	int done = 0;
	int last_row = 0;
	while(TRUE) {
		error = SMSDSQL_Query(Config, SBPtr(buf), &res);
		if(error != ERR_NONE)
			break;

		int rows_fetched = (int)db->AffectedRows(Config, &res);
		int current_row = 0;

		if(rows_fetched)
			while(db->NextRow(Config, &res)) {
				if(current_row < last_row) {
					current_row++;
					continue;
				}
				const char *media_type = db->GetString(Config, &res, 0);
				ssize_t data_length = db->GetNumber(Config, &res, 1);
				CPTR data = db->GetString(Config, &res, 2);
				done = (int)db->GetNumber(Config, &res, 3);
				MMSMessage_AddPart(m, media_type, data, data_length);
			}

		if(done)
			break;

		last_row = rows_fetched;

		usleep(800000);
		if (time(NULL) > timeout) {
			error = ERR_TIMEOUT;
			break;
		}
	}

	db->FreeResult(Config, &res);
	SB_Destroy(&buf);

	if(error != ERR_NONE) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to retrieve all MMS parts from database");
		return error;
	}

	SB_Clear(MMSBuffer);
	MMS_EncodeMessage(MMSBuffer, m);
	MMSMessage_Destroy(&m);

	Config->MMSSendInfo.outboxID = outbox_id;
	Config->MMSSendInfo.mmsTxID = txid;

	return error;
}

/* Find one multi SMS to sending and return it (or return ERR_EMPTY)
 * There is also set ID for SMS
 */
/*static*/ GSM_Error SMSDSQL_FindOutboxSMS(GSM_MultiSMSMessage * sms, GSM_SMSDConfig * Config, char *ID)
{
	SQL_result res;
	struct GSM_SMSDdbobj *db = Config->db;
	int i;
	gboolean last;
	time_t timestamp;
	const char *coding;
	const char *text;
	size_t text_len;
	const char *text_decoded;
	const char *destination = NULL;
	const char *udh;
	const char *q;
	const char *status;
	const char *mms_headers;
	long outbox_id;
	size_t udh_len;
	SQL_Var vars[3];
	GSM_Error error;

	vars[0].type = SQL_TYPE_INT;
	vars[0].v.i = 1;
	vars[1].type = SQL_TYPE_NONE;

	while (TRUE) {
		error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_FIND_OUTBOX_SMS_ID], NULL, NULL, vars, &res, FALSE);
		if (error != ERR_NONE) {
			SMSD_Log(DEBUG_INFO, Config, "Error reading from database (%s)", __FUNCTION__);
			return error;
		}

		if (db->NextRow(Config, &res) != 1) {
			db->FreeResult(Config, &res);
			return ERR_EMPTY;
		}

		outbox_id = (long)db->GetNumber(Config, &res, 0);
		sprintf(ID, "%ld", outbox_id);
		timestamp = db->GetDate(Config, &res, 1);

		db->FreeResult(Config, &res);

		if (timestamp == -1) {
			SMSD_Log(DEBUG_INFO, Config, "Invalid date for InsertIntoDB.");
			return ERR_UNKNOWN;
		}

		SMSDSQL_Time2String(Config, timestamp, Config->DT, sizeof(Config->DT));
		if (SMSDSQL_RefreshSendStatus(Config, ID) == ERR_NONE) {
			break;
		}
	}

	sms->Number = 0;
	for (i = 0; i < GSM_MAX_MULTI_SMS; i++) {
		GSM_SetDefaultSMSData(&sms->SMS[i]);
		/* Force using default SMSC */
		sms->SMS[i].SMSC.Location = 0;
	}

	for (i = 1; i < GSM_MAX_MULTI_SMS + 1; i++) {
		vars[0].type = SQL_TYPE_STRING;
		vars[0].v.s = ID;
		vars[1].type = SQL_TYPE_INT;
		vars[1].v.i = i;
		vars[2].type = SQL_TYPE_NONE;
		if (i == 1) {
			q = Config->SMSDSQL_queries[SQL_QUERY_FIND_OUTBOX_BODY];
		} else {
			q = Config->SMSDSQL_queries[SQL_QUERY_FIND_OUTBOX_MULTIPART];
		}
		error = SMSDSQL_NamedQuery(Config, q, NULL, NULL, vars, &res, FALSE);
		if (error != ERR_NONE) {
			SMSD_Log(DEBUG_ERROR, Config, "Error reading from database (%s)", __FUNCTION__);
			return error;
		}

		if (db->NextRow(Config, &res) != 1) {
			db->FreeResult(Config, &res);
			return ERR_NONE;
		}

		status = db->GetString(Config, &res, i == 1 ? 12 : 7);
		if (status != NULL && strncmp(status, "SendingOK", 9) == 0) {
			SMSD_Log(DEBUG_NOTICE, Config, "Marking %s:%d message for skip", ID, i);
			Config->SkipMessage[sms->Number] = TRUE;
		} else {
			Config->SkipMessage[sms->Number] = FALSE;
		}

		text = db->GetString(Config, &res, 0);
		if (text == NULL) {
			text_len = 0;
		} else {
			text_len = strlen(text);
		}
		udh = db->GetString(Config, &res, 2);
		int Class = (char)db->GetNumber(Config, &res, 3);
		sms->SMS[sms->Number].Class = Class;
		text_decoded = db->GetString(Config, &res, 4);
		if (udh == NULL) {
			udh_len = 0;
		} else {
			udh_len = strlen(udh);
		}

		/* ID, we don't need it, but some ODBC backend need to fetch all values */
		db->GetNumber(Config, &res, 5);

    coding = db->GetString(Config, &res, 1);
		if(coding && strncasecmp("network_default", coding, 15) == 0) {
      sms->SMS[sms->Number].Coding = GSM_NetworkDefaultCoding(Config->gsm->CurrentConfig);
    }
		else {
      sms->SMS[sms->Number].Coding = GSM_StringToSMSCoding(coding);
    }

		if (sms->SMS[sms->Number].Coding == 0) {
			if (text == NULL || text_len == 0) {
				SMSD_Log(DEBUG_NOTICE, Config, "Assuming default coding for text message");
				sms->SMS[sms->Number].Coding = GSM_NetworkDefaultCoding(Config->gsm->CurrentConfig);
			} else {
				SMSD_Log(DEBUG_NOTICE, Config, "Assuming 8bit coding for binary message");
				sms->SMS[sms->Number].Coding = SMS_Coding_8bit;
			}
		}

		if (Class != GSM_CLASS_MMS) {
			if (text == NULL || text_len == 0) {
				if (text_decoded == NULL) {
					SMSD_Log(DEBUG_ERROR, Config, "Message without text!");
					return ERR_UNKNOWN;
				} else {
					SMSD_Log(DEBUG_NOTICE, Config, "Message: %s", text_decoded);
					DecodeUTF8(sms->SMS[sms->Number].Text, text_decoded, strlen(text_decoded));
				}
			}
			else {
				switch (sms->SMS[sms->Number].Coding) {
					case SMS_Coding_Unicode_No_Compression:
					case SMS_Coding_Default_No_Compression:
					case SMS_Coding_ASCII:
						if (! DecodeHexUnicode(sms->SMS[sms->Number].Text, text, text_len)) {
							SMSD_Log(DEBUG_ERROR, Config, "Failed to decode Text HEX string: %s", text);
							return ERR_UNKNOWN;
						}
						break;

					case SMS_Coding_8bit:
						if (!DecodeHexBin(sms->SMS[sms->Number].Text, text, text_len)) {
							SMSD_Log(DEBUG_ERROR, Config, "Failed to decode Text HEX string: %s", text);
							return ERR_UNKNOWN;
						}
						sms->SMS[sms->Number].Length = text_len / 2;
						break;

					default:
						break;
				}
			}
		}

		if (i == 1) {
			destination = db->GetString(Config, &res, 6);
			if (destination == NULL) {
				SMSD_Log(DEBUG_ERROR, Config, "Message without recipient!");
				return ERR_UNKNOWN;
			}
			DecodeUTF8(sms->SMS[sms->Number].Number, destination, strlen(destination));

			Config->currdeliveryreport = db->GetBool(Config, &res, 9);
			if(Config->currdeliveryreport == -1 && strcmp(Config->deliveryreport, "no") != 0)
				Config->currdeliveryreport = 1;
		} else {
			CopyUnicodeString(sms->SMS[sms->Number].Number, sms->SMS[0].Number);
		}

		sms->SMS[sms->Number].PDU = Config->currdeliveryreport == 1 ? SMS_Status_Report : SMS_Submit;

		if(i == 1 &&  Class == GSM_CLASS_MMS) {
			mms_headers = db->GetString(Config, &res, 14);
			error = SMSDSQL_PrepareOutboxMMS(Config, outbox_id, destination, mms_headers, Config->MMSBuffer);
			if(error != ERR_NONE && error != ERR_EMPTY) {
				db->FreeResult(Config, &res);
				return error;
			}

			if(error != ERR_EMPTY)
				error = MMS_MESSAGE_TO_SEND;
		}

		sms->SMS[sms->Number].UDH.Type = UDH_NoUDH;
		if (udh != NULL && udh_len != 0 && error != MMS_MESSAGE_TO_SEND) {
			sms->SMS[sms->Number].UDH.Type = UDH_UserUDH;
			sms->SMS[sms->Number].UDH.Length = (int)udh_len / 2;
			if (! DecodeHexBin(sms->SMS[sms->Number].UDH.Text, udh, udh_len)) {
				SMSD_Log(DEBUG_ERROR, Config, "Failed to decode UDH HEX string: %s", udh);
				return ERR_UNKNOWN;
			}
			GSM_DecodeUDHHeader(GSM_GetDI(Config->gsm), &sms->SMS[sms->Number].UDH);
		}

		sms->Number++;

		if (i == 1) {
			/* Is this a multipart message? */
			last = !db->GetBool(Config, &res, 7);
			Config->relativevalidity = (int)db->GetNumber(Config, &res, 8);

			strncpy(Config->CreatorID, db->GetString(Config, &res, 10), sizeof(Config->CreatorID));
			Config->CreatorID[sizeof(Config->CreatorID) - 1] = 0;
			Config->retries = (int)db->GetNumber(Config, &res, 11);
		}
		db->FreeResult(Config, &res);
		if (last) {
			last = FALSE;
			break;
		}
	}

	return error;
}

/* After sending SMS is moved to Sent Items or Error Items. */
static GSM_Error SMSDSQL_MoveSMS(GSM_MultiSMSMessage * sms UNUSED, GSM_SMSDConfig * Config, char *ID, gboolean alwaysDelete UNUSED, gboolean sent UNUSED)
{
	SQL_result res;
	SQL_Var vars[2];
	struct GSM_SMSDdbobj *db = Config->db;
	GSM_Error error;

	vars[0].type = SQL_TYPE_STRING;
	vars[0].v.s = ID;
	vars[1].type = SQL_TYPE_NONE;

	error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_DELETE_OUTBOX], NULL, NULL, vars, &res, FALSE);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_INFO, Config, "Error deleting from database (%s)", __FUNCTION__);
		return error;
	}
	db->FreeResult(Config, &res);

	error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_DELETE_OUTBOX_MULTIPART], NULL, NULL, vars, &res, FALSE);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_INFO, Config, "Error deleting from database (%s)", __FUNCTION__);
		return error;
	}
	db->FreeResult(Config, &res);

	return ERR_NONE;
}

/* Adds SMS to Outbox */
GSM_Error SMSDSQL_CreateOutboxSMS(GSM_MultiSMSMessage * sms, GSM_SMSDConfig * Config, char *NewID)
{
	char creator[200];
	int i;
	unsigned long long ID = 0;
	SQL_result res;
	SQL_Var vars[6];
	struct GSM_SMSDdbobj *db = Config->db;
	const char *report, *multipart, *q;
	GSM_Error error;

	sprintf(creator, "Gammu %s",GAMMU_VERSION); /* %1 */
	multipart = (sms->Number == 1) ? "FALSE" : "TRUE"; /* %3 */

	for (i = 0; i < sms->Number; i++) {
		report = (sms->SMS[i].PDU == SMS_Status_Report) ? "yes": "default"; /* %2 */
		if (i == 0) {
			q = Config->SMSDSQL_queries[SQL_QUERY_CREATE_OUTBOX];
		} else {
			q = Config->SMSDSQL_queries[SQL_QUERY_CREATE_OUTBOX_MULTIPART];
		}

		vars[0].type = SQL_TYPE_STRING;
		vars[0].v.s = creator;
		vars[1].type = SQL_TYPE_STRING;
		vars[1].v.s = report;
		vars[2].type = SQL_TYPE_STRING;
		vars[2].v.s = multipart;
		vars[3].type = SQL_TYPE_INT;
		vars[3].v.i = i+1;
		vars[4].type = SQL_TYPE_INT;
		vars[4].v.i = ID;
		vars[5].type = SQL_TYPE_NONE;

		error = SMSDSQL_NamedQuery(Config, q, &sms->SMS[i], NULL, vars, &res, FALSE);
		if (error != ERR_NONE) {
			SMSD_Log(DEBUG_INFO, Config, "Error writing to database (%s)", __FUNCTION__);
			return error;
		}
		if (i == 0) {
			ID = db->SeqID(Config, "outbox_ID_seq");
			if (ID == 0) {
				SMSD_Log(DEBUG_INFO, Config, "Failed to get inserted row ID (%s)", __FUNCTION__);
				return ERR_UNKNOWN;
			}
		}
		db->FreeResult(Config, &res);
	}
	SMSD_Log(DEBUG_INFO, Config, "Written message with ID %llu", ID);
	if (NewID != NULL)
		sprintf(NewID, "%llu", ID);
	return ERR_NONE;
}

GSM_Error MoveSentMMSParts(GSM_SMSDConfig *Config, long long outbox_id)
{
	assert(Config);
	GSM_Error error;
	SQL_result res;
	SBUFFER buf = SB_InitWithCapacity(4096);

	SB_PutFormattedString(buf, "with sent_parts as (delete from outbox_mms_parts a where \"OUTBOX_ID\" = %lld returning a.*) ", outbox_id);
	SB_PutString(buf, "insert into sentitems_mms_parts select * from sent_parts;");
	SB_PutByte(buf, 0);
	error = SMSDSQL_Query(Config, SBBase(buf), &res);
	SB_Destroy(&buf);
	Config->db->FreeResult(Config, &res);

	if (error != ERR_NONE)
		SMSD_Log(DEBUG_ERROR, Config, "Error moving sent MMS parts (%s)", __FUNCTION__);

	return error;
}

GSM_Error SMSDSQL_AddSentMMSInfo(GSM_SMSDConfig *Config, CSTR Coding, CSTR TextDecoded, CSTR MmsHeaders)
{
	GSM_Error error;
	SQL_result res;
	MMSMESSAGE sendConf = Config->MMSSendInfo.sendConf;
	MMSHEADER messageId = NULL;
	MMSHEADER sendStatus = NULL;
	CSTR statusText = NULL;
	SBUFFER buf = SB_Init();

	if(sendConf) {
		messageId = MMSMessage_FindHeader(sendConf, MMS_HEADER, MMS_MESSAGE_ID);
		if(!messageId)
			SMSD_Log(DEBUG_INFO, Config, "Got server response without message id field.");

		sendStatus = MMSMessage_FindHeader(sendConf, MMS_HEADER, MMS_RESPONSE_STATUS);
		if(!sendStatus)
			SMSD_Log(DEBUG_INFO, Config, "Got server response without status field.");
	}

	if(sendStatus) {
		SMSD_Log(DEBUG_NOTICE, Config, "MMS SendStatus: %s",	sendStatus->value.v.enum_v->name);
		if(sendStatus->value.v.enum_v->code != MMS_RESP_OK)
			statusText = "Error";
	}

	SB_PutFormattedString(buf, "update sentitems set \"StatusCode\" = %d, \"MMS_ID\" = '", Config->StatusCode);

	if(messageId)
		SB_PutBytes(buf, messageId->value.v.str, strlen(messageId->value.v.str));
	else
		SB_PutAsBinHex(buf, &Config->MMSSendInfo.mmsTxID, sizeof(Config->MMSSendInfo.mmsTxID));

	if(statusText)
		SB_PutFormattedString(buf, "', \"Status\" = '%s", statusText);

	SB_PutFormattedString(buf, "', \"MMSHeaders\" = '%s', \"Coding\" = '%s', \"TextDecoded\" = '%s'  where \"ID\" = %lld;",
		MmsHeaders, Coding, TextDecoded, Config->MMSSendInfo.outboxID);
	SB_PutByte(buf, 0);

	error = SMSDSQL_Query(Config, SBBase(buf), &res);
	SB_Destroy(&buf);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_INFO, Config, "Error writing to database (%s)", __FUNCTION__);
		return error;
	}
	Config->db->FreeResult(Config, &res);

	error = MoveSentMMSParts(Config, Config->MMSSendInfo.outboxID);

	Config->MMSSendInfo.outboxID = -1;
	Config->MMSSendInfo.mmsTxID = -1;

	return error;
}

static GSM_Error SMSDSQL_AddSentSMSInfo(GSM_MultiSMSMessage * sms, GSM_SMSDConfig * Config, char *ID, int Part, GSM_SMSDSendingError err, int TPMR)
{
	SQL_result res, r2;
	struct GSM_SMSDdbobj *db = Config->db;
	GSM_Error error;
	size_t query_type;

	const char *message_state;
	SQL_Var vars[7];
	char smsc[GSM_MAX_NUMBER_LENGTH + 1];
	char destination[GSM_MAX_NUMBER_LENGTH + 1];

	EncodeUTF8(smsc, sms->SMS[Part - 1].SMSC.Number);
	EncodeUTF8(destination, sms->SMS[Part - 1].Number);

	if (err == SMSD_SEND_OK) {
		SMSD_Log(DEBUG_NOTICE, Config, "Transmitted %s (%s: %i) to %s", Config->SMSID,
			 (Part == sms->Number ? "total" : "part"), Part, DecodeUnicodeString(sms->SMS[0].Number));

		if (sms->SMS[Part - 1].PDU == SMS_Status_Report) {
			message_state = "SendingOK";
		} else {
			message_state = "SendingOKNoReport";
		}
	} else if (err == SMSD_SEND_SENDING_ERROR) {
		message_state = "SendingError";
	} else if (err == SMSD_SEND_ERROR) {
		message_state = "Error";
	} else {
		SMSD_Log(DEBUG_INFO, Config, "Unknown SMS state: %d, assuming Error", err);
		message_state = "Error";
	}

	/* 1 = ID, 2 = SequencePosition, 3 = Status, 4 = TPMR, 5 = insertintodb */
	vars[0].type = SQL_TYPE_STRING;
	vars[0].v.s = ID;
	vars[1].type = SQL_TYPE_INT;
	vars[1].v.i = Part;
	vars[2].type = SQL_TYPE_STRING;
	vars[2].v.s = message_state;
	vars[3].type = SQL_TYPE_INT;
	vars[3].v.i = TPMR;
	vars[4].type = SQL_TYPE_STRING;
	vars[4].v.s = Config->DT;
	vars[5].type = SQL_TYPE_NONE;

	query_type = (Part == 1) ? SQL_QUERY_FIND_OUTBOX_BODY : SQL_QUERY_FIND_OUTBOX_MULTIPART;
	error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[query_type], NULL, NULL, vars, &res, FALSE);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_ERROR, Config, "Error reading from database (%s)", __FUNCTION__);
		return error;
	}
	if (db->NextRow(Config, &res) != 1) {
		db->FreeResult(Config, &res);
		return ERR_NONE;
	}
	/* 6 = StatusCode */
	vars[5].type = SQL_TYPE_INT;
	vars[5].v.i = (int)db->GetNumber(Config, &res, Part == 1 ? 13 : 8);
	vars[6].type = SQL_TYPE_NONE;

	int Class = (char)db->GetNumber(Config, &res, 3);
	CSTR Coding = db->GetString(Config, &res, 1);
	CSTR TextDecoded = db->GetString(Config, &res, 4);
	CSTR MmsHeaders = NULL;

	error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_ADD_SENT_INFO], &sms->SMS[Part - 1], NULL, vars, &r2, FALSE);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_INFO, Config, "Error writing to database (%s)", __FUNCTION__);
		return error;
	}
	db->FreeResult(Config, &r2);

	if(Part == 1 && Class == GSM_CLASS_MMS) {
		MmsHeaders = db->GetString(Config, &res, 14);
		SMSDSQL_AddSentMMSInfo(Config, Coding, TextDecoded, MmsHeaders);
	}

	error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_UPDATE_SENT], &sms->SMS[Part - 1], NULL, NULL, &res, FALSE);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_INFO, Config, "Error updating number of sent messages (%s)", __FUNCTION__);
		return error;
	}
	db->FreeResult(Config, &res);

	if (sms->Number != 1) {
		query_type = (Part == 1) ? SQL_QUERY_UPDATE_OUTBOX : SQL_QUERY_UPDATE_OUTBOX_MULTIPART;
		error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[query_type], &sms->SMS[Part - 1], NULL, vars, &res, FALSE);
		if (error != ERR_NONE) {
			SMSD_Log(DEBUG_INFO, Config, "Error updating status of multipart messages (%s)", __FUNCTION__);
			return error;
		}
		db->FreeResult(Config, &res);
	}

	return ERR_NONE;
}

static GSM_Error SMSDSQL_RefreshPhoneStatus(GSM_SMSDConfig * Config)
{
	SQL_result res;
	SQL_Var vars[3] = {
		{SQL_TYPE_INT, {NULL}},
		{SQL_TYPE_INT, {NULL}},
		{SQL_TYPE_NONE, {NULL}}};
	struct GSM_SMSDdbobj *db = Config->db;
	GSM_Error error;

	vars[0].v.i = Config->Status->Charge.BatteryPercent;
	vars[1].v.i = Config->Status->Network.SignalPercent;

	error = SMSDSQL_NamedQuery(Config, Config->SMSDSQL_queries[SQL_QUERY_REFRESH_PHONE_STATUS], NULL, NULL, vars, &res, FALSE);
	if (error != ERR_NONE) {
		SMSD_Log(DEBUG_INFO, Config, "Error writing to database (%s)", __FUNCTION__);
		return error;
	}
	db->FreeResult(Config, &res);

	return ERR_NONE;
}

/*
 * better strcat... shows where is the bug
 */
#define STRCAT_MAX 80
GSM_Error SMSDSQL_option(GSM_SMSDConfig *Config, int optint, const char *option, ...)
{
	size_t len[STRCAT_MAX], to_alloc = 0;
	int i, j;
	va_list ap;
	const char *arg;
	const char *args[STRCAT_MAX];
	char *buffer, *ptr;

	/* read from config */
	buffer = INI_GetValue(Config->smsdcfgfile, "sql", option, FALSE);
	/* found? */
	if (buffer != NULL){
		Config->SMSDSQL_queries[optint] = strdup(buffer); /* avoid to double free */
		return ERR_NONE;
	}

	/* not found.. we use default query */
	va_start(ap, option);
	for(i = 0; i < STRCAT_MAX; i++){
		arg = va_arg(ap, const char *);
		if (arg == NULL)
			break;
		len[i] = strlen(arg);
		args[i] = arg;
		to_alloc += len[i];
	}
	va_end(ap);

	if (i == STRCAT_MAX) {
		SMSD_Log(DEBUG_ERROR, Config, "STRCAT_MAX too small.. consider increase this value for option %s", option);
		return ERR_UNKNOWN;
	}

	buffer = malloc(to_alloc+1);
	if (buffer == NULL){
		SMSD_Log(DEBUG_ERROR, Config, "Insufficient memory problem for option %s", option);
		return ERR_UNKNOWN;
	}
	ptr = buffer;
	for (j = 0; j < i; j++) {
		memcpy(ptr, args[j], len[j]);
		ptr += len[j];
	}
	*ptr = '\0';
	Config->SMSDSQL_queries[optint] = buffer;
	return ERR_NONE;
}


/**
 * Reads common options for database backends.
 */
GSM_Error SMSDSQL_ReadConfiguration(GSM_SMSDConfig *Config)
{
	int locktime;
	const char *escape_char;

	Config->user = INI_GetValue(Config->smsdcfgfile, "smsd", "user", FALSE);
	if (Config->user == NULL) {
		Config->user="root";
	}

	Config->password = INI_GetValue(Config->smsdcfgfile, "smsd", "password", FALSE);
	if (Config->password == NULL) {
		Config->password="";
	}

	Config->host = INI_GetValue(Config->smsdcfgfile, "smsd", "host", FALSE);
	if (Config->host == NULL) {
		/* Backward compatibility */
		Config->host = INI_GetValue(Config->smsdcfgfile, "smsd", "pc", FALSE);
	}
	if (Config->host == NULL) {
		Config->host="localhost";
	}

	Config->database = INI_GetValue(Config->smsdcfgfile, "smsd", "database", FALSE);
	if (Config->database == NULL) {
		Config->database="sms";
	}

	Config->table_gammu = INI_GetValue(Config->smsdcfgfile, "tables", "gammu", FALSE);
	if (Config->table_gammu == NULL){
		Config->table_gammu = "gammu";
	}
	Config->table_inbox = INI_GetValue(Config->smsdcfgfile, "tables", "inbox", FALSE);
	if (Config->table_inbox == NULL){
		Config->table_inbox = "inbox";
	}
	Config->table_sentitems = INI_GetValue(Config->smsdcfgfile, "tables", "sentitems", FALSE);
	if (Config->table_sentitems == NULL){
		Config->table_sentitems = "sentitems";
	}
	Config->table_outbox = INI_GetValue(Config->smsdcfgfile, "tables", "outbox", FALSE);
	if (Config->table_outbox == NULL){
		Config->table_outbox = "outbox";
	}
	Config->table_outbox_multipart = INI_GetValue(Config->smsdcfgfile, "tables", "outbox_multipart", FALSE);
	if (Config->table_outbox_multipart == NULL){
		Config->table_outbox_multipart = "outbox_multipart";
	}
	Config->table_phones = INI_GetValue(Config->smsdcfgfile, "tables", "phones", FALSE);
	if (Config->table_phones == NULL){
		Config->table_phones = "phones";
	}

	Config->driverspath = INI_GetValue(Config->smsdcfgfile, "smsd", "driverspath", FALSE);

	Config->sql = INI_GetValue(Config->smsdcfgfile, "smsd", "sql", FALSE);

	Config->dbdir = INI_GetValue(Config->smsdcfgfile, "smsd", "dbdir", FALSE);

	if (Config->driver == NULL) {
		SMSD_Log(DEBUG_ERROR, Config, "No database driver selected. Must be native_mysql, native_pgsql, ODBC or DBI one.");
		return ERR_UNKNOWN;
	}

	Config->db = NULL;
	if (!strcasecmp(Config->driver, "native_mysql")) {
#ifdef HAVE_MYSQL_MYSQL_H
		Config->db = &SMSDMySQL;
#else
		SMSD_Log(DEBUG_ERROR, Config, "The %s driver was not compiled in!", Config->driver);
		return ERR_DISABLED;
#endif
	}
	if (!strcasecmp(Config->driver, "native_pgsql")) {
#ifdef HAVE_POSTGRESQL_LIBPQ_FE_H
		Config->db = &SMSDPgSQL;
#else
		SMSD_Log(DEBUG_ERROR, Config, "The %s driver was not compiled in!", Config->driver);
		return ERR_DISABLED;
#endif
	}
	if (!strcasecmp(Config->driver, "odbc")) {
#ifdef ODBC_FOUND
		Config->db = &SMSDODBC;
		if (Config->sql == NULL) {
			SMSD_Log(DEBUG_INFO, Config, "Using generic SQL for ODBC, this might fail. In such case please set SQL configuration option.");
		}
#else
		SMSD_Log(DEBUG_ERROR, Config, "The %s driver was not compiled in!", Config->driver);
		return ERR_DISABLED;
#endif
	}
	if (Config->db == NULL) {
#ifdef LIBDBI_FOUND
		Config->db = &SMSDDBI;
#else
		SMSD_Log(DEBUG_ERROR, Config, "Unknown DB driver");
		return ERR_UNKNOWN;
#endif
	}

	escape_char = SMSDSQL_EscapeChar(Config);
#define ESCAPE_FIELD(x) escape_char, x, escape_char

	locktime = Config->loopsleep * 8; /* reserve 8 sec per message */
	locktime = locktime < 60 ? 60 : locktime; /* Minimum time reserve is 60 sec */

	if (SMSDSQL_option(Config, SQL_QUERY_DELETE_PHONE, "delete_phone",
		"DELETE FROM ", Config->table_phones, " WHERE ", ESCAPE_FIELD("IMEI"), " = %I", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_INSERT_PHONE, "insert_phone",
		"INSERT INTO ", Config->table_phones, " (",
			ESCAPE_FIELD("IMEI"),
			", ", ESCAPE_FIELD("IMSI"),
			", ", ESCAPE_FIELD("ID"),
			", ", ESCAPE_FIELD("NetCode"),
			", ", ESCAPE_FIELD("NetName"),
			", ", ESCAPE_FIELD("Send"),
			", ", ESCAPE_FIELD("Receive"),
			", ", ESCAPE_FIELD("InsertIntoDB"),
			", ", ESCAPE_FIELD("TimeOut"),
			", ", ESCAPE_FIELD("Client"),
			", ", ESCAPE_FIELD("Battery"),
			", ", ESCAPE_FIELD("Signal"),
			") VALUES (%I, %S, %P, %O, %M, %1, %2, ",
			SMSDSQL_Now(Config),
			", ",
			SMSDSQL_NowPlus(Config, 10),
			", %N, -1, -1)", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_SAVE_INBOX_SMS_SELECT, "save_inbox_sms_select",
		"SELECT ",
			ESCAPE_FIELD("ID"),
			", ", ESCAPE_FIELD("Status"),
			", ", ESCAPE_FIELD("SendingDateTime"),
			", ", ESCAPE_FIELD("DeliveryDateTime"),
			", ", ESCAPE_FIELD("SMSCNumber"), " "
			"FROM ", Config->table_sentitems, " WHERE ",
			ESCAPE_FIELD("DeliveryDateTime"), " IS NULL AND ",
			ESCAPE_FIELD("SenderID"), " = %P AND ",
			ESCAPE_FIELD("TPMR"), " = %t AND ",
			ESCAPE_FIELD("DestinationNumber"), " = %R", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_SAVE_INBOX_SMS_UPDATE_DELIVERED, "save_inbox_sms_update_delivered",
		"UPDATE ", Config->table_sentitems, " "
			"SET ", ESCAPE_FIELD("DeliveryDateTime"), " = %C"
			", ", ESCAPE_FIELD("Status"), " = %1"
			", ", ESCAPE_FIELD("StatusError"), " = %e"
			" WHERE ", ESCAPE_FIELD("ID"), " = %2"
			" AND ", ESCAPE_FIELD("TPMR"), " = %t", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_SAVE_INBOX_SMS_UPDATE, "save_inbox_sms_update",
		"UPDATE ", Config->table_sentitems, " "
			"SET ",	ESCAPE_FIELD("Status"), " = %1"
			", ", ESCAPE_FIELD("StatusError"), " = %e"
			" WHERE ", ESCAPE_FIELD("ID"), " = %2"
			" AND ", ESCAPE_FIELD("TPMR"), " = %t", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_SAVE_INBOX_SMS_INSERT, "save_inbox_sms_insert",
		"INSERT INTO ", Config->table_inbox, " "
			"(", ESCAPE_FIELD("ReceivingDateTime"),
			", ", ESCAPE_FIELD("Text"),
			", ", ESCAPE_FIELD("SenderNumber"),
			", ", ESCAPE_FIELD("Coding"),
			", ", ESCAPE_FIELD("SMSCNumber"),
			", ", ESCAPE_FIELD("UDH"),
			", ", ESCAPE_FIELD("Class"),
			", ", ESCAPE_FIELD("TextDecoded"),
			", ", ESCAPE_FIELD("RecipientID"),
			", ", ESCAPE_FIELD("Status"), ")",
			" VALUES (%d, %E, %R, %c, %F, %u, %x, %T, %P, %e)", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_UPDATE_RECEIVED, "update_received",
		"UPDATE ", Config->table_phones, " SET ",
			ESCAPE_FIELD("Received"), " = ", ESCAPE_FIELD("Received"), " + 1"
			" WHERE ", ESCAPE_FIELD("IMEI"), " = %I", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_REFRESH_SEND_STATUS, "refresh_send_status",
		"UPDATE ", Config->table_outbox, " SET ",
			ESCAPE_FIELD("SendingTimeOut"), " = ", SMSDSQL_NowPlus(Config, locktime),
			" WHERE ", ESCAPE_FIELD("ID"), " = %1"
			" AND (", ESCAPE_FIELD("SendingTimeOut"), " < ", SMSDSQL_Now(Config),
			" OR ", ESCAPE_FIELD("SendingTimeOut"), " IS NULL)", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_UPDATE_RETRIES, "update_retries",
		"UPDATE ", Config->table_outbox, " SET ",
			ESCAPE_FIELD("SendingTimeOut"), " = ", SMSDSQL_NowPlus(Config, Config->retrytimeout),
			", ",ESCAPE_FIELD("Retries"), " = %2"
			" WHERE ", ESCAPE_FIELD("ID"), " = %1", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_FIND_OUTBOX_SMS_ID, "find_outbox_sms_id",
		"SELECT ", SMSDSQL_TopClause(Config, "%1"),
			ESCAPE_FIELD("ID"),
			", ", ESCAPE_FIELD("InsertIntoDB"),
			", ", ESCAPE_FIELD("SendingDateTime"),
			", ", ESCAPE_FIELD("SenderID"),
			" FROM ", Config->table_outbox, " WHERE ",
			ESCAPE_FIELD("SendingDateTime"), " < ", SMSDSQL_Now(Config),
			" AND ", ESCAPE_FIELD("SendingTimeOut"), " < ", SMSDSQL_Now(Config),
			" AND ", ESCAPE_FIELD("SendBefore"), " >= ", SMSDSQL_CurrentTime(Config),
			" AND ", ESCAPE_FIELD("SendAfter"), " <= ", SMSDSQL_CurrentTime(Config),
			" AND ( ", ESCAPE_FIELD("SenderID"), " is NULL OR ", ESCAPE_FIELD("SenderID"), " = '' OR ", ESCAPE_FIELD("SenderID"), " = %P )",
			SMSDSQL_RownumClause(Config, "%1", TRUE),
			" ORDER BY ", ESCAPE_FIELD("Priority"), " DESC, ", ESCAPE_FIELD("InsertIntoDB"), " ASC ", SMSDSQL_LimitClause(Config, "%1"), NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_FIND_OUTBOX_BODY, "find_outbox_body",
		"SELECT ",
			ESCAPE_FIELD("Text"),
			", ", ESCAPE_FIELD("Coding"),
			", ", ESCAPE_FIELD("UDH"),
			", ", ESCAPE_FIELD("Class"),
			", ", ESCAPE_FIELD("TextDecoded"),
			", ", ESCAPE_FIELD("ID"),
			", ", ESCAPE_FIELD("DestinationNumber"),
			", ", ESCAPE_FIELD("MultiPart"),
			", ", ESCAPE_FIELD("RelativeValidity"),
			", ", ESCAPE_FIELD("DeliveryReport"),
			", ", ESCAPE_FIELD("CreatorID"),
			", ", ESCAPE_FIELD("Retries"),
			", ", ESCAPE_FIELD("Status"),
			", ", ESCAPE_FIELD("StatusCode"),
			", ", ESCAPE_FIELD("MMSHeaders"),
			" FROM ", Config->table_outbox, " WHERE ",
			ESCAPE_FIELD("ID"), "=%1", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_FIND_OUTBOX_MULTIPART, "find_outbox_multipart",
		"SELECT ",
			ESCAPE_FIELD("Text"),
			", ", ESCAPE_FIELD("Coding"),
			", ", ESCAPE_FIELD("UDH"),
			", ", ESCAPE_FIELD("Class"),
			", ", ESCAPE_FIELD("TextDecoded"),
			", ", ESCAPE_FIELD("ID"),
			", ", ESCAPE_FIELD("SequencePosition"),
			", ", ESCAPE_FIELD("Status"),
			", ", ESCAPE_FIELD("StatusCode"),
			" FROM ", Config->table_outbox_multipart, " WHERE ",
			ESCAPE_FIELD("ID"), "=%1 AND ",
			ESCAPE_FIELD("SequencePosition"), "=%2", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_DELETE_OUTBOX, "delete_outbox",
		"DELETE FROM ", Config->table_outbox, " WHERE ", ESCAPE_FIELD("ID"), "=%1", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_DELETE_OUTBOX_MULTIPART, "delete_outbox_multipart",
		"DELETE FROM ", Config->table_outbox_multipart, " WHERE ", ESCAPE_FIELD("ID"), "=%1", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_CREATE_OUTBOX, "create_outbox",
		"INSERT INTO ", Config->table_outbox, " "
			"(", ESCAPE_FIELD("CreatorID"),
			", ", ESCAPE_FIELD("SenderID"),
			", ", ESCAPE_FIELD("DeliveryReport"),
			", ", ESCAPE_FIELD("MultiPart"),
			", ", ESCAPE_FIELD("InsertIntoDB"),
			", ", ESCAPE_FIELD("Text"),
			", ", ESCAPE_FIELD("DestinationNumber"),
			", ", ESCAPE_FIELD("RelativeValidity"),
			", ", ESCAPE_FIELD("Coding"),
			", ", ESCAPE_FIELD("UDH"),
			", ", ESCAPE_FIELD("Class"),
			", ", ESCAPE_FIELD("TextDecoded"), ") VALUES "
			"(%1, %P, %2, %3, ", SMSDSQL_Now(Config),
			", %E, %R, %V, %c, %u, %x, %T)", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_CREATE_OUTBOX_MULTIPART, "create_outbox_multipart",
		"INSERT INTO ", Config->table_outbox_multipart, " "
			"(", ESCAPE_FIELD("SequencePosition"),
			", ", ESCAPE_FIELD("Text"),
			", ", ESCAPE_FIELD("Coding"),
			", ", ESCAPE_FIELD("UDH"),
			", ", ESCAPE_FIELD("Class"),
			", ", ESCAPE_FIELD("TextDecoded"),
			", ", ESCAPE_FIELD("ID"), ") VALUES (%4, %E, %c, %u, %x, %T, %5)", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_UPDATE_OUTBOX, "update_outbox",
		"UPDATE ", Config->table_outbox, " SET ",
			ESCAPE_FIELD("Status"), "=%3 WHERE ",
			ESCAPE_FIELD("ID"), "=%1", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_UPDATE_OUTBOX_MULTIPART, "update_outbox_multipart",
		"UPDATE ", Config->table_outbox_multipart, " SET ",
			ESCAPE_FIELD("Status"), "=%3 WHERE ",
			ESCAPE_FIELD("ID"), "=%1 AND ",
			ESCAPE_FIELD("SequencePosition"), "=%2", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_UPDATE_OUTBOX_STATUSCODE, "update_outbox_statuscode",
		"UPDATE ", Config->table_outbox, " SET ",
			ESCAPE_FIELD("StatusCode"), "=%3 WHERE ",
			ESCAPE_FIELD("ID"), "=%1", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_UPDATE_OUTBOX_MULTIPART_STATUSCODE, "update_outbox_multipart_statuscode",
		"UPDATE ", Config->table_outbox_multipart, " SET ",
			ESCAPE_FIELD("StatusCode"), "=%3 WHERE ",
			ESCAPE_FIELD("ID"), "=%1 AND ",
			ESCAPE_FIELD("SequencePosition"), "=%4", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_ADD_SENT_INFO, "add_sent_info",
		"INSERT INTO ", Config->table_sentitems, " "
			"(", ESCAPE_FIELD("CreatorID"),
			", ", ESCAPE_FIELD("ID"),
			", ", ESCAPE_FIELD("SequencePosition"),
			", ", ESCAPE_FIELD("Status"),
			", ", ESCAPE_FIELD("SendingDateTime"),
			", ", ESCAPE_FIELD("SMSCNumber"),
			", ", ESCAPE_FIELD("TPMR"),
			", ", ESCAPE_FIELD("SenderID"),
			", ", ESCAPE_FIELD("Text"),
			", ", ESCAPE_FIELD("DestinationNumber"),
			", ", ESCAPE_FIELD("Coding"),
			", ", ESCAPE_FIELD("UDH"),
			", ", ESCAPE_FIELD("Class"),
			", ", ESCAPE_FIELD("TextDecoded"),
			", ", ESCAPE_FIELD("InsertIntoDB"),
			", ", ESCAPE_FIELD("RelativeValidity"),
			", ", ESCAPE_FIELD("StatusCode"),
			") "
			" VALUES (%A, %1, %2, %3, ",
			SMSDSQL_Now(Config),
			", %F, %4, %P, %E, %R, %c, %u, %x, %T, %5, %V, %6)", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_UPDATE_SENT, "update_sent",
		"UPDATE ", Config->table_phones, " SET ",
			ESCAPE_FIELD("Sent"), "= ", ESCAPE_FIELD("Sent"), " + 1"
			" WHERE ", ESCAPE_FIELD("IMEI"), " = %I", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}

	if (SMSDSQL_option(Config, SQL_QUERY_REFRESH_PHONE_STATUS, "refresh_phone_status",
		"UPDATE ", Config->table_phones, " SET ",
			ESCAPE_FIELD("TimeOut"), "= ", SMSDSQL_NowPlus(Config, 10),
			", ", ESCAPE_FIELD("Battery"), " = %1"
			", ", ESCAPE_FIELD("Signal"), " = %2"
			", ", ESCAPE_FIELD("NetCode"), " = %O"
			", ", ESCAPE_FIELD("NetName"), " = %M"
			" WHERE ", ESCAPE_FIELD("IMEI"), " = %I", NULL) != ERR_NONE) {
		return ERR_UNKNOWN;
	}
#undef ESCAPE_FIELD

	return ERR_NONE;
}

/* Converts the given local date and time into POSIX calendar time
 *
 * The date string argument must be a system local point in time
 * formatted as "YYYY-MM-DD HH:MM:SS"
 *
 * returns the POSIX (UTC) calendar time for the given date/time, or
 * a negative time_t on error.
 */
time_t SMSDSQL_ParseDate(GSM_SMSDConfig * Config, const char *date)
{
	char *parse_res;
	struct tm tm;
	time_t time = -1;

	if (strcmp(date, "0000-00-00 00:00:00") == 0) {
		return -2;
	}

	parse_res = strptime(date, "%Y-%m-%d %H:%M:%S", &tm);
	if (parse_res != NULL && *parse_res == 0) {
	  tm.tm_isdst = -1;
	  time = mktime(&tm);
	}
	else if (Config != NULL) {
    /* Used during testing */
		SMSD_Log(DEBUG_ERROR, Config, "Failed to parse date: %s", date);
	}

	return time;
}

GSM_SMSDService SMSDSQL = {
	SMSDSQL_Init,
	SMSDSQL_Free,
	SMSDSQL_InitAfterConnect,
	SMSDSQL_SaveInboxSMS,
	SMSDSQL_FindOutboxSMS,
	SMSDSQL_MoveSMS,
	SMSDSQL_CreateOutboxSMS,
	SMSDSQL_AddSentSMSInfo,
	SMSDSQL_RefreshSendStatus,
	SMSDSQL_UpdateRetries,
	SMSDSQL_RefreshPhoneStatus,
	SMSDSQL_ReadConfiguration
};

/* How should editor hadle tabs in this file? Add editor commands here.
 * vim: noexpandtab sw=8 ts=8 sts=8:
 */
