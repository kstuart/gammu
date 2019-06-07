#include <assert.h>
#include "test_helper.h"
#include "../../libgammu/gsmstate.h"
#include "../../smsd/core.h"
#include "../../smsd/services/files.h"

GSM_Error ATGEN_GetSMS(GSM_StateMachine *s, GSM_MultiSMSMessage *sms);

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
    "+CPMS: (\"ME\"),(\"ME\")\r",
    "OK\r\n",
    "+CPMS: 1,99,1,99\r",
    "OK\r\n",
    "+CMGR: 0,,25\r",
    "088010415847221664190530095939100200020004E9979F40\r",
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

  error = ATGEN_GetSMS(s, &msms);
  test_result(error == ERR_NONE);
  test_result(sms->Length == 4);
  test_result(sms->Coding == SMS_Coding_ASCII);
  test_result(strcmp("test", DecodeUnicodeString(sms->Text)) == 0);

  cleanup_state_machine(s);
}

