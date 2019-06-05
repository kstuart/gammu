#include <assert.h>
#include "test_helper.h"
#include "../../libgammu/gsmstate.h"
#include "../../smsd/core.h"
#include "../../smsd/services/files.h"

gboolean SMSD_ReadDeleteSMS(GSM_SMSDConfig *Config);

GSM_Error SMSD_ConfigureLogging(GSM_SMSDConfig *Config, gboolean uselog);
void SMSD_IncomingSMSCallback(GSM_StateMachine *s, GSM_SMSMessage *sms, void *user_data);

int main(void)
{
  GSM_Error error;
  GSM_StateMachine *s = NULL;
  GSM_SMSDStatus status;
  GSM_SMSDConfig *config = SMSD_NewConfig("test");

  const char *responses[] = {
    // AT+CPMS=?
    "+CPMS: (\"ME\"),(\"ME\")\r",
    "OK\r\n",
    // AT+CPMS="ME"
    "+CPMS: 2,99,2,99\r",
    "OK\r\n",
    // AT+CPMS="ME","ME"
    "+CPMS: 2,99,2,99\r",
    "OK\r\n",
    // AT+CMGL=4
    "+CMGL: 1,1,\"\",25\r",
    "088010415847221664190530095939100200020004E9979F40\r",
    "+CMGL: 2,2,\“\”,23\r",
    "07801091346554F307801096224658F110020000000A61616161616161616161\r",
    "OK\r\n",
    // AT+CPMS="ME","ME"
    "+CPMS: 1,99,1,99\r",
    "OK\r\n",
    // AT+CMGL=4
    "+CMGL: 1,1,\"\",25\r",
    "088010415847221664190530095939100200020004E9979F40\r",
    "OK\r\n",
    // AT+CPMS="ME"
    "+CPMS: 1,99,1,99\r",
    "OK\r\n",
    // AT+CMGR=1
    "+CMGR: 0,,25\r",
    "088010415847221664190530095939100200020004E9979F40\r",
    "OK\r\n",
    // AT+CPMS="ME","ME"
    "+CPMS: 1,99,1,99\r",
    "OK\r\n",
    // AT+CMGD=1
    "OK\r\n",
    // AT+CPMS="ME"
    "+CPMS: 0,99,0,99\r",
    "OK\r\n",
    // AT+CMGR=1
    "ERROR\r\n",

    // Sentinel
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

  error = SMSD_ReadDeleteSMS(config);
  test_result(error == ERR_NONE);

  cleanup_state_machine(s);
}

