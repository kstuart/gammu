#include <string.h>
#include <dynamic-buffer.h>
#include "mms-data.h"

MMSVALUEENUM MMSEnumLookupByName(MMSVALUEENUM tbl, int count, CSTR name)
{
	for(int i = 0; i < count; i++)
		if(strcmp(name, tbl[i].name) == 0)
			return &tbl[i];

	return NULL;
}

MMSVALUEENUM MMSEnumLookupByID(MMSVALUEENUM tbl, int count, MMSShortInt value)
{
	for(int i = 0; i < count; i++)
		if(tbl[i].code == value)
			return &tbl[i];

	return NULL;
}


MMSVALUEENUM MMSEncodedEnumQuickLookup(MMSVALUEENUM tbl, int count, MMSShortInt value)
{
	MMSShortInt idx = value & 0x7fu;
	if(idx > count || idx < 0)
		return NULL;

	MMSVALUEENUM entry = &tbl[idx];
	if(entry->code != value)
		return NULL;

	return entry;
}

MMSError _EnumLookup(CDBUFFER stream, MMSVALUE out, MMSValueType vt, MMSVALUEENUM tbl, int count)
{
	MMSShortInt code;
	code = DynaBuf_NextByte(stream);

	MMSVALUEENUM entry = MMSEncodedEnumQuickLookup(tbl, count, code);
	if(!entry)
		return MME_OUTOFRANGE;

	return MMSValue_SetEnum(out, vt, entry);
}


MMSFieldInfo WSPFields[] = {
	{WSP_ACCEPT,               "Accept",               VT_UNSUPPORTED},
	{WSP_ACCEPT_CHARSET,       "Accept-Charset",       VT_UNSUPPORTED},
	{WSP_ACCEPT_ENCODING,      "Accept-Encoding",      VT_UNSUPPORTED},
	{WSP_ACCEPT_LANGUAGE,      "Accept-Language",      VT_UNSUPPORTED},
	{WSP_ACCEPT_RANGES,        "Accept-Ranges",        VT_UNSUPPORTED},
	{WSP_AGE,                  "Age",                  VT_UNSUPPORTED},
	{WSP_ALLOW,                "Allow",                VT_UNSUPPORTED},
	{WSP_AUTHORIZATION,        "Authorization",        VT_UNSUPPORTED},
	{WSP_CACHE_CONTROL,        "Cache-Control",        VT_UNSUPPORTED},
	{WSP_CONNECTION,           "Connection",           VT_UNSUPPORTED},
	{WSP_CONTENT_BASE,         "Content-Base",         VT_UNSUPPORTED},
	{WSP_CONTENT_ENCODING,     "Content-Encoding",     VT_UNSUPPORTED},
	{WSP_CONTENT_LANGUAGE,     "Content-Language",     VT_UNSUPPORTED},
	{WSP_CONTENT_LENGTH,       "Content-Length",       VT_UNSUPPORTED},
	{WSP_CONTENT_LOCATION,     "Content-Location",     VT_UNSUPPORTED},
	{WSP_CONTENT_MD5,          "Content-MD5",          VT_UNSUPPORTED},
	{WSP_CONTENT_RANGE,        "Content-Range",        VT_UNSUPPORTED},
	{WSP_CONTENT_TYPE,         "Content-Type",         VT_UNSUPPORTED},
	{WSP_DATE,                 "Date",                 VT_UNSUPPORTED},
	{WSP_ETAG,                 "Etag",                 VT_UNSUPPORTED},
	{WSP_EXPIRES,              "Expires",              VT_UNSUPPORTED},
	{WSP_FROM,                 "From",                 VT_UNSUPPORTED},
	{WSP_HOST,                 "Host",                 VT_UNSUPPORTED},
	{WSP_IF_MODIFIED_SINCE,    "If-Modified-Since",    VT_UNSUPPORTED},
	{WSP_IF_MATCH,             "If-Match",             VT_UNSUPPORTED},
	{WSP_IF_NONE_MATCH,        "If-None-Match",        VT_TEXT},
	{WSP_IF_RANGE,             "If-Range",             VT_UNSUPPORTED},
	{WSP_IF_UNMODIFIED_SINCE,  "If-Unmodified-Since",  VT_UNSUPPORTED},
	{WSP_LOCATION,             "Location",             VT_UNSUPPORTED},
	{WSP_LAST_MODIFIED,        "Last-Modified",        VT_UNSUPPORTED},
	{WSP_MAX_FORWARDS,         "Max-Forwards",         VT_UNSUPPORTED},
	{WSP_PRAGMA,               "Pragma",               VT_UNSUPPORTED},
	{WSP_PROXY_AUTHENTICATE,   "Proxy-Authenticate",   VT_UNSUPPORTED},
	{WSP_PROXY_AUTHORIZATION,  "Proxy-Authorization",  VT_UNSUPPORTED},
	{WSP_PUBLIC,               "Public",               VT_UNSUPPORTED},
	{WSP_RANGE,                "Range",                VT_UNSUPPORTED},
	{WSP_REFERER,              "Referer",              VT_UNSUPPORTED},
	{WSP_RETRY_AFTER,          "Retry-After",          VT_UNSUPPORTED},
	{WSP_SERVER,               "Server",               VT_UNSUPPORTED},
	{WSP_TRANSFER_ENCODING,    "Transfer-Encoding",    VT_UNSUPPORTED},
	{WSP_UPGRADE,              "Upgrade",              VT_UNSUPPORTED},
	{WSP_USER_AGENT,           "User-Agent",           VT_UNSUPPORTED},
	{WSP_VARY,                 "Vary",                 VT_UNSUPPORTED},
	{WSP_VIA,                  "Via",                  VT_UNSUPPORTED},
	{WSP_WARNING,              "Warning",              VT_UNSUPPORTED},
	{WSP_WWW_AUTHENTICATE,     "WWW-Authenticate",     VT_UNSUPPORTED},
	{WSP_CONTENT_DISPOSITION,  "Content-Disposition",  VT_UNSUPPORTED},
	{WSP_X_WAP_APPLICATION_ID, "X-Wap-Application-Id", VT_UNSUPPORTED},
	{WSP_X_WAP_CONTENT_URI,    "X-Wap-Content-URI",    VT_UNSUPPORTED},
	{WSP_X_WAP_INITIATOR_URI,  "X-Wap-Initiator-URI",  VT_UNSUPPORTED},
	{WSP_ACCEPT_APPLICATION,   "Accept-Application",   VT_UNSUPPORTED},
	{WSP_BEARER_INDICATION,    "Bearer-Indication",    VT_UNSUPPORTED},
	{WSP_PUSH_FLAG,            "Push-Flag",            VT_UNSUPPORTED},
	{WSP_PROFILE,              "Profile",              VT_UNSUPPORTED},
	{WSP_PROFILE_DIFF,         "Profile-Diff",         VT_UNSUPPORTED},
	{WSP_PROFILE_WARNING,      "Profile-Warning",      VT_UNSUPPORTED},
};

MMSFIELDINFO WSPFields_FindByID(int id)
{
	static const size_t limit = sizeof(WSPFields) / sizeof(WSPFields[0]);
	MMSFIELDINFO fi;
	if ((unsigned) id > limit)
		return NULL;

	fi = &WSPFields[id];

	return fi->id == id ? fi : NULL;
}

MMSFieldInfo MMSFields[] = {
	{MMS_BCC,               "Bcc",                     VT_ENCODED_STRING},
	{MMS_CC,                "Cc",                      VT_ENCODED_STRING},
	{MMS_CONTENT_LOCATION,  "X-Mms-Content-Location",  VT_TEXT},
	{MMS_CONTENT_TYPE,      "Content-Type",            VT_CONTENT_TYPE},
	{MMS_DATE,              "Date",                    VT_LONG_INT},
	{MMS_DELIVERY_REPORT,   "X-Mms-Delivery-Report",   VT_YESNO},
	{MMS_DELIVERY_TIME,     "X-Mms-Delivery-Time",     VT_DTIME},
	{MMS_EXPIRY,            "X-Mms-Expiry",            VT_DTIME},
	{MMS_FROM,              "From",                    VT_FROM},
	{MMS_MESSAGE_CLASS,     "X-Mms-Message-Class",     VT_MESSAGE_CLASS},
	{MMS_MESSAGE_ID,        "Message-ID",              VT_TEXT},
	{MMS_MESSAGE_TYPE,      "X-Mms-Message-Type",      VT_MESSAGE_TYPE},
	{MMS_MMS_VERSION,       "X-Mms-MMS-Version",       VT_SHORT_INT},
	{MMS_MESSAGE_SIZE,      "X-Mms-Message-Size",      VT_LONG_INT},
	{MMS_PRIORITY,          "X-Mms-Priority",          VT_PRIORITY},
	{MMS_READ_REPORT,       "X-Mms-Read-Report",       VT_UNSUPPORTED},
	{MMS_REPORT_ALLOWED,    "X-Mms-Report-Allowed",    VT_UNSUPPORTED},
	{MMS_RESPONSE_STATUS,   "X-Mms-Response-Status",   VT_RESPONSE_STATUS},
	{MMS_RESPONSE_TEXT,     "X-Mms-Response-Text",     VT_ENCODED_STRING},
	{MMS_SENDER_VISIBILITY, "X-Mms-Sender-Visibility", VT_HIDESHOW},
	{MMS_STATUS,            "X-Mms-Status",            VT_STATUS},
	{MMS_SUBJECT,           "Subject",                 VT_ENCODED_STRING},
	{MMS_TO,                "To",                      VT_ENCODED_STRING},
	{MMS_TRANSACTION_ID,    "X-Mms-Transaction-Id",    VT_TEXT},
};

MMSFIELDINFO MMSFields_FindByID(int id)
{
	static const size_t limit = sizeof(MMSFields) / sizeof(MMSFields[0]);
	MMSFIELDINFO fi;
	if ((unsigned) id > limit)
		return NULL;

	fi = &MMSFields[id - 1];

	return fi->id == id ? fi : NULL;
}

// http://openmobilealliance.org/wp/OMNA/wsp/wsp-header-parameters.html
MMSFieldInfo MMSWellKnownParams[] = {
	{0x00, "Q",           VT_UNSUPPORTED},
	{0x01, "charset",     VT_WK_CHARSET},
	{0x02, "level",       VT_UNSUPPORTED},
	{0x03, "type",        VT_UNSUPPORTED},
	{0x04, "uaprof",      VT_UNSUPPORTED},
	{0x05, "name",        VT_UNSUPPORTED},
	{0x06, "filename",    VT_UNSUPPORTED},
	{0x07, "differences", VT_UNSUPPORTED},
	{0x08, "padding",     VT_UNSUPPORTED},
};

MMSFIELDINFO MMSWellKnownParams_FindByID(int id)
{
	static const size_t limit = sizeof(MMSWellKnownParams) / sizeof(MMSWellKnownParams[0]);
	MMSFIELDINFO fi;
	if ((unsigned) id > limit)
		return NULL;

	fi = &MMSWellKnownParams[id];

	return fi->id == id ? fi : NULL;
}


// https://www.iana.org/assignments/character-sets/character-sets.xhtml
MMSValueEnum MMSCharsetEnum[] = {
	{"*",        0},
	{"US-ASCII", 3},
	{"UTF-8",    106}
};
size_t MMSCharsetEnumSize = sizeof(MMSCharsetEnum) / sizeof(MMSCharsetEnum[0]);

MMSVALUEENUM Charset_FindByID(int id)
{
	return MMSEnumLookupByID(MMSCharsetEnum, MMSCharsetEnumSize, id);
}

// http://openmobilealliance.org/wp/OMNA/wsp/wsp_content_type_codes.html
MMSValueEnum WKContentTypes[] = {
	{"*/*",                                       0x00},
	{"text/*",                                    0x01},
	{"text/html",                                 0x02},
	{"text/plain",                                0x03},
	{"text/x-hdml",                               0x04},
	{"text/x-ttml",                               0x05},
	{"text/x-vCalendar",                          0x06},
	{"text/x-vCard",                              0x07},
	{"text/vnd.wap.wml",                          0x08},
	{"text/vnd.wap.wmlscript",                    0x09},
	{"text/vnd.wap.wta-event",                    0x0A},
	{"multipart/mixed",                           0x0C},
	{"multipart/form-data",                       0x0D},
	{"multipart/byterantes",                      0x0E},
	{"multipart/alternative",                     0x0F},
	{"application/*",                             0x10},
	{"application/java-vm",                       0x11},
	{"application/x-www-form-urlencoded",         0x12},
	{"application/x-hdmlc",                       0x13},
	{"application/vnd.wap.wmlc",                  0x14},
	{"application/vnd.wap.wmlscriptc",            0x15},
	{"application/vnd.wap.wta-eventc",            0x16},
	{"application/vnd.wap.uaprof",                0x17},
	{"application/vnd.wap.wtls-ca-certificate",   0x18},
	{"application/vnd.wap.wtls-user-certificate", 0x19},
	{"application/x-x509-ca-cert",                0x1A},
	{"application/x-x509-user-cert",              0x1B},
	{"image/*",                                   0x1C},
	{"image/gif",                                 0x1D},
	{"image/jpeg",                                0x1E},
	{"image/tiff",                                0x1F},
	{"image/png",                                 0x20},
	{"image/vnd.wap.wbmp",                        0x21},
	{"application/vnd.wap.multipart.*",           0x22},
	{"application/vnd.wap.multipart.mixed",       0x23},
	{"application/vnd.wap.multipart.form-data",   0x24},
	{"application/vnd.wap.multipart.byteranges",  0x25},
	{"application/vnd.wap.multipart.alternative", 0x26},
	{"application/xml",                           0x27},
	{"text/xml",                                  0x28},
	{"application/vnd.wap.wbxml",                 0x29},
	{"application/x-x968-cross-cert",             0x2A},
	{"application/x-x968-ca-cert",                0x2B},
	{"application/x-x968-user-cert",              0x2C},
	{"text/vnd.wap.si",                           0x2D},
	{"application/vnd.wap.sic",                   0x2E},
	{"text/vnd.wap.sl",                           0x2F},
	{"application/vnd.wap.slc",                   0x30},
	{"text/vnd.wap.co",                           0x31},
	{"application/vnd.wap.coc",                   0x32},
	{"application/vnd.wap.multipart.related",     0x33},
	{"application/vnd.wap.sia",                   0x34},
	{"text/vnd.wap.connectivity-xml",             0x35},
	{"application/vnd.wap.connectivity-wbxml",    0x36},
	{"application/pkcs7-mime",                    0x37},
	{"application/vnd.wap.hashed-certificate",    0x38},
	{"application/vnd.wap.cert-response",         0x3A},
	{"application/xhtml+xml",                     0x3B},
	{"application/wml+xml",                       0x3C},
	{"text/css",                                  0x3D},
	{"application/vnd.wap.mms-message",           0x3E},
	{"application/vnd.wap.rollover-certificate",  0x3F},
	{"application/vnd.wap.locc+wbxml",            0x40},
	{"application/vnd.wap.loc+xml",               0x41},
	{"application/vnd.syncml.dm+wbxml",           0x42},
	{"application/vnd.syncml.dm+xml",             0x43},
	{"application/vnd.syncml.notification",       0x44},
	{"application/vnd.wap.xhtml+xml",             0x45},
	{"application/vnd.wv.csp.cir",                0x46},
	{"application/vnd.oma.dd+xml",                0x47},
	{"application/vnd.oma.drm.message",           0x48},
	{"application/vnd.oma.drm.content",           0x49},
	{"application/vnd.oma.drm.rights+xml",        0x4A},
	{"application/vnd.oma.drm.rights+wbxml",      0x4B},
	{"application/vnd.wv.csp+xml",                0x4C},
	{"application/vnd.wv.csp+wbxml",              0x4D},
	{"application/vnd.syncml.ds.notification",    0x4E},
	{"audio/*",                                   0x4F},
	{"video/*",                                   0x50},
};
size_t WkContentTypesSize = sizeof(WKContentTypes) / sizeof(WKContentTypes[0]);

MMSVALUEENUM MMS_WkContentType_FindByID(int id)
{
	return MMSEnumLookupByID(WKContentTypes, WkContentTypesSize, id);
}