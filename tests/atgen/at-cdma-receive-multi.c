#include <assert.h>
#include "test_helper.h"
#include "../../libgammu/gsmstate.h"
#include "../../smsd/core.h"
#include "../../smsd/services/files.h"

GSM_Error SMSD_Init(GSM_SMSDConfig *Config);
gboolean SMSD_CheckSMSStatus(GSM_SMSDConfig *Config);

GSM_Error SMSD_ConfigureLogging(GSM_SMSDConfig *Config, gboolean uselog);
void SMSD_IncomingSMSCallback(GSM_StateMachine *s, GSM_SMSMessage *sms, void *user_data);

int main(void)
{
  GSM_Error error;
  GSM_StateMachine *s = NULL;
  GSM_Phone_ATGENData *Priv;
  GSM_SMSDStatus status;
  GSM_SMSDConfig *config = SMSD_NewConfig("test");

  const char *responses[] = {
    // AT+CPMS=?
    "+CPMS: (\"ME\"),(\"ME\")\r",
    "OK\r\n",
    // AT+CPMS="ME","ME"
    "+CPMS: 2,99,2,99\r",
    "OK\r\n",
    // AT+CPMS="ME","ME"
    "+CPMS: 2,99,2,99\r",
    "OK\r\n",
    // AT+CMGL=4
    "+CMGL: 1,0,\"\",180\r",
    "0880104178039355181906120814231005000901A0050003620201414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141\r",
    "+CMGL: 2,0,\"\",44\r",
    "0880104178039355181906120814241005000901180500036202024646464646464646464646464646414141\r",
    "OK\r\n",
    // AT+CPMS="ME"
    "+CPMS: 2,99,2,99\r",
    "OK\r\n",
    // AT+CMGR=1
    "+CMGR: 1,\"\",180\r",
    "0880104178039355181906120814231005000901A0050003620201414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141\r",
    "OK\r\n",
    // AT+CPMS="ME","ME"
    "+CPMS: 2,99,2,99\r",
    "OK\r\n",
    // AT+CPMS="ME"
    "+CPMS: 2,99,2,99\r",
    "OK\r\n",
    // AT+CMGD=1
    "OK\r\n",
    // AT+CMGR=1
    "ERROR\r\n",

    // AT+CMGR=2
    "+CMGR: 1,\"\",44\r",
    "0880104178039355181906120814241005000901180500036202024646464646464646464646464646414141\r",
    "OK\r\n",
    // AT+CPMS="ME","ME"
    "+CPMS: 1,99,1,99\r",
    "OK\r\n",
    // AT+CPMS="ME"
    "+CPMS: 1,99,1,99\r",
    "OK\r\n",
    // AT+CMGD=1
    "OK\r\n",
    // AT+CMGR=2
    "ERROR\r\n",

    // Sentinel
    "ERROR\r\n"
  };
  SET_RESPONSES(responses);

  error = SMSD_ReadConfig("smsd-cdma.cfg", config, TRUE);
  test_result(error == ERR_NONE);
  SMSD_EnableGlobalDebug(config);

  s = config->gsm;
  Priv = &s->Phone.Data.Priv.ATGEN;

  GSM_SetDebugGlobal(TRUE, GSM_GetDebug(s));
  setup_at_engine(s);
  bind_response_handling(s);

  memset(&status, 0, sizeof(GSM_SMSDStatus));
  config->Status = &status;

  Priv->SMSMemory = MEM_ME;

  error = SMSD_Init(config);
  test_result(error == ERR_NONE);

  error = SMSD_CheckSMSStatus(config);
  test_result(error == ERR_NONE);

  cleanup_state_machine(s);
}

