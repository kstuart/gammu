#include <assert.h>
#include <curl/curl.h>
#include "log.h"
#include "core.h"

#include "streambuffer.h"
#include "mms-service.h"

#include "mms-mobiledata.h"

gboolean SMSD_RunOn(const char*, GSM_MultiSMSMessage*, GSM_SMSDConfig*, const char*, const char*);

GSM_Error MobileDataStart(GSM_SMSDConfig *Config)
{
	assert(Config != NULL);
	if(Config->RunOnDataConnect == NULL) {
		SMSD_Log(DEBUG_INFO, Config, "No RunOnDataConnect script provided to register APN.");
		SMSD_Log(DEBUG_INFO, Config, "Assuming the connection is already established.");
		return ERR_NONE;
	}

	gboolean success = SMSD_RunOn(Config->RunOnDataConnect, NULL, Config, "start", "data connect");
	if(success == FALSE) {
		SMSD_Log(DEBUG_ERROR, Config, "Start APN Registration script failed.");
		return ERR_ABORTED;
	}
	return ERR_NONE;
}

GSM_Error MobileDataStop(GSM_SMSDConfig *Config)
{
	assert(Config);

	if(Config->RunOnDataConnect == NULL) {
		SMSD_Log(DEBUG_INFO, Config, "No RunOnDataConnect script provided to unregister APN.");
		SMSD_Log(DEBUG_INFO, Config, "Assuming the connection is managed externally.");
		return ERR_NONE;
	}

	gboolean success = SMSD_RunOn(Config->RunOnDataConnect, NULL, Config, "stop", "data disconnect");
	if(success == FALSE) {
		SMSD_Log(DEBUG_ERROR, Config, "Stop APN Registration script failed.");
		return ERR_ABORTED;
	}
	return ERR_NONE;
}

static size_t ReceiveBufferCallback(void *in_ptr, size_t size, size_t in_count, void *arg)
{
	SBUFFER buffer = (SBUFFER)arg;
	assert(size == 1);

	SB_PutBytes(buffer, in_ptr, in_count);
	return in_count;
}

size_t SendBufferCallback(void *ptr, size_t size, size_t nitems, void *arg) {
	SBUFFER buffer = (SBUFFER)arg;
	ssize_t bytes_read = SB_GetBytes(buffer, ptr, size * nitems);

	return bytes_read < 0 ? 0 : bytes_read;
}

static int CURL_DebugLogger(CURL *ch, curl_infotype ci,  char *data, size_t size, void *arg)
{
	assert(arg);
	char buffer[4096];
	GSM_SMSDConfig *Cfg = (GSM_SMSDConfig*)arg;

	if(size > 4095)
		size = 4095;

	memcpy(buffer, data, size);
	buffer[size] = '\0';

	switch(ci) {
		default:
			return 0;
		case CURLINFO_HEADER_OUT:
			SMSD_Log(DEBUG_NOTICE, Cfg, "Request Header: %s", buffer);
			break;
		case CURLINFO_HEADER_IN:
			SMSD_Log(DEBUG_NOTICE, Cfg, "Response Header: %s", buffer);
			break;
	}
	return 0;
}

static GSM_Error CURL_GetFromURL(GSM_SMSDConfig *Config, SBUFFER Buffer, const char *URL)
{
	CURL *ch;
	CURLcode cr;

	ch = curl_easy_init();
	if(!ch) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to create CURL context");
		return ERR_ABORTED;
	}

	SMSD_Log(DEBUG_INFO, Config, "Performing HTTP Get with URL %s", URL);
	curl_easy_setopt(ch, CURLOPT_URL, URL);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, ReceiveBufferCallback);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void*)Buffer);
	if(Config->MMSCProxy) {
		curl_easy_setopt(ch, CURLOPT_PROXY, Config->MMSCProxy);
	}
	if(Config->debug_level > 0) {
		curl_easy_setopt(ch, CURLOPT_DEBUGFUNCTION, CURL_DebugLogger);
		curl_easy_setopt(ch, CURLOPT_DEBUGDATA, Config);
		curl_easy_setopt(ch, CURLOPT_VERBOSE, 1L);
	}
	cr = curl_easy_perform(ch);

	if(cr != CURLE_OK)
		SMSD_Log(DEBUG_ERROR, Config, "Failed to fetch URL from server: %s", curl_easy_strerror(cr));

	/* cleanup curl stuff */
	curl_easy_cleanup(ch);

	return cr == CURLE_OK ? ERR_NONE : ERR_ABORTED;
}

static GSM_Error CURL_PostToURL(GSM_SMSDConfig *Config, SBUFFER Buffer, SBUFFER RespBuffer, const char *URL)
{
	CURL *ch;
	CURLcode cr;

	if(!RespBuffer) {
		SMSD_LogErrno(Config, "Failed to create response buffer");
		return ERR_MEMORY;
	}

	ch = curl_easy_init();
	if(!ch) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to create CURL context");
		return ERR_ABORTED;
	}

	struct curl_slist *list = NULL;
	list = curl_slist_append(list, "Content-Type: application/vnd.wap.mms-message");
	curl_easy_setopt(ch, CURLOPT_HTTPHEADER, list);

	curl_easy_setopt(ch, CURLOPT_URL, URL);
	curl_easy_setopt(ch, CURLOPT_POST, 1L);
	curl_easy_setopt(ch, CURLOPT_READFUNCTION, SendBufferCallback);
	curl_easy_setopt(ch, CURLOPT_READDATA, (void*)Buffer);
	curl_easy_setopt(ch, CURLOPT_POSTFIELDSIZE, (long)SBUsed(Buffer));
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, ReceiveBufferCallback);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void*)RespBuffer);
	if(Config->MMSCProxy) {
		curl_easy_setopt(ch, CURLOPT_PROXY, Config->MMSCProxy);
	}
	if(Config->debug_level > 0) {
		curl_easy_setopt(ch, CURLOPT_DEBUGFUNCTION, CURL_DebugLogger);
		curl_easy_setopt(ch, CURLOPT_DEBUGDATA, Config);
		curl_easy_setopt(ch, CURLOPT_VERBOSE, 1L);
	}
	cr = curl_easy_perform(ch);


	if(cr != CURLE_OK)
		SMSD_Log(DEBUG_ERROR, Config, "Failed to post MMS to server: %s", curl_easy_strerror(cr));

	/* cleanup curl stuff */
	curl_easy_cleanup(ch);

	return cr == CURLE_OK ? ERR_NONE : ERR_ABORTED;
}

GSM_Error FetchMMS(GSM_SMSDConfig *Config, SBUFFER Buffer, GSM_MMSIndicator *MMSIndicator)
{
	assert(Config);
	assert(Buffer);
	assert(MMSIndicator);

	GSM_Error error = MobileDataStart(Config);
	if(error != ERR_NONE) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to establish APN network.");
		return error;
	}

	error = CURL_GetFromURL(Config, Buffer, MMSIndicator->Address);
	SMSD_Log(DEBUG_INFO, Config, "%lu bytes retrieved from server.", SBUsed(Buffer));

	if(MobileDataStop(Config) != ERR_NONE)
		SMSD_Log(DEBUG_ERROR, Config, "Failed to disconnect APN network.");

	return error;
}

GSM_Error SendMMS(GSM_SMSDConfig *Config, SBUFFER Buffer)
{
	assert(Config);
	assert(Buffer);
	SBUFFER RespBuffer = SB_Init();

	GSM_Error error = MobileDataStart(Config);
	if(error != ERR_NONE) {
		SMSD_Log(DEBUG_ERROR, Config, "Failed to establish APN network.");
		return error;
	}

	error = CURL_PostToURL(Config, Buffer, RespBuffer, Config->MMSCAddress);

	if(MobileDataStop(Config) != ERR_NONE)
		SMSD_Log(DEBUG_ERROR, Config, "Failed to disconnect APN network.");

	if(SBUsed(RespBuffer))
		SMSD_ProcessServerResponse(Config, RespBuffer);

	SB_Destroy(&RespBuffer);
	return error;
}

MMSConveyor MMSMobileDataConveyor = {
	FetchMMS,
	SendMMS,
};
