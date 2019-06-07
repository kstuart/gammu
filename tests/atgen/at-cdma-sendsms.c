#include <assert.h>
#include "test_helper.h"
#include "../../libgammu/gsmstate.h"
#include "../../smsd/core.h"
#include "../../smsd/services/files.h"

GSM_Error ATGEN_SendSMS(GSM_StateMachine *s, GSM_SMSMessage *sms);

GSM_Error SMSD_ConfigureLogging(GSM_SMSDConfig *Config, gboolean uselog);
void SMSD_IncomingSMSCallback(GSM_StateMachine *s, GSM_SMSMessage *sms, void *user_data);

int main(void)
{
  GSM_Error error;
  GSM_MultiSMSMessage msms;
  GSM_SMSMessage *sms = &msms.SMS[0];
  GSM_StateMachine *s = NULL;
  GSM_SMSDStatus status;
  GSM_SMSDConfig *config = SMSD_NewConfig("test");

  const char *responses[] = {
    "\r\n> ",
    "+CMGS=30\r",
    "OK\r\n",

    "ERROR\r\n"
  };
  SET_RESPONSES(responses);

  error = SMSD_ReadConfig("smsd-cdma.cfg", config, TRUE);
  test_result(error == ERR_NONE);
  SMSD_EnableGlobalDebug(config);

  s = config->gsm;
  GSM_SetDebugGlobal(TRUE, GSM_GetDebug(s));
  setup_at_engine(s);
  bind_response_handling(s);

  memset(&status, 0, sizeof(GSM_SMSDStatus));
  config->Status = &status;

  memset(&msms, 0, sizeof(msms));

  char msg[] = "aaaaaaaaaabbbbbbbbbb";

  EncodeUnicode(sms->Number, "01194356453", 11);
  EncodeUnicode(sms->Text, msg, strlen(msg));
  sms->Length = UnicodeLength(sms->Text);
  sms->PDU = SMS_Submit;
  sms->SMSC = config->SMSC;
  //sms->Coding = SMS_Coding_Default_No_Compression;
  sms->Coding = SMS_Coding_ASCII;
  //sms->Coding = SMS_Coding_8bit;

  error = ATGEN_SendSMS(s, sms);
  test_result(error == ERR_NONE);

  cleanup_state_machine(s);
}

