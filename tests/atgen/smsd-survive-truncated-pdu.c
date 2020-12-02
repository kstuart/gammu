#include <assert.h>
#include "test_helper.h"
#include "../../libgammu/gsmstate.h"
#include "../../smsd/core.h"

GSM_Error SMSD_Init(GSM_SMSDConfig *Config);
gboolean SMSD_CheckSMSStatus(GSM_SMSDConfig *Config);

/*
 * Test against #378 - All SMS received twice
 */
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
    "+CPMS: 4,99,4,99\r",
    "OK\r\n",
    // AT+CPMS="ME","ME"
    "+CPMS: 4,99,4,99\r",
    "OK\r\n",

    // AT+CMGL=4
    "+CMGL: 1,1,,104\r",
	  "0791322230000000200D91327430053248F10000021111616254406061333C222FC3DF727A283C1FC1466DF2480693C15AB1582B161BB5D3E411C6\r",
	  "+CMGL: 2,1,,25\r",
	  "0791328220000000000D91329455724892F700000211115131004005C8329BFD06\r",
	  "+CMGL: 3,1,,104\r",
	  "0791329930000000240D91327761075587F70000021140902173406061333C222FC3DF727A283C1FC1466DF2480693C15A31580B961BB5D3E411C0\r",
    "+CMGL: 4,1,\"\",23\r",
    "07918444483252F0040B918446445078F700007121320144744004D4F29C0E\r",
    "OK\r\n",

    // AT+CPMS="ME"
    "+CPMS: 4,99,4,99\r",
    "OK\r\n",
    // AT+CMGR=1
    "+CMGR: 1,1,,104\r",
    "0791322230000000200D91327430053248F10000021111616254406061333C222FC3DF727A283C1FC1466DF2480693C15AB1582B161BB5D3E411C6\r",
    "OK\r\n",
    // AT+CPMS="ME","ME"
    "+CPMS: 4,99,4,99\r",
    "OK\r\n",
    // AT+CMGD=1
    "OK\r\n",

	  // AT+CPMS="ME"
    "+CPMS: 3,99,3,99\r",
    "OK\r\n",
	  // AT+CMGR=2
    "+CMGR: 2,1,,25\r",
    "0791328220000000000D91329455724892F700000211115131004005C8329BFD06\r",
    "OK\r\n",
	  // AT+CPMS="ME","ME"
    "+CPMS: 3,99,3,99\r",
    "OK\r\n",
	  // AT+CMGD=1
    "OK\r\n",

	  // AT+CPMS="ME"
    "+CPMS: 2,99,2,99\r",
    "OK\r\n",
	  // AT+CMGR=2
    "+CMGR: 3,1,,104\r",
    "0791329930000000240D91327761075587F70000021140902173406061333C222FC3DF727A283C1FC1466DF2480693C15A31580B961BB5D3E411C0\r",
    "OK\r\n",
	  // AT+CPMS="ME","ME"
    "+CPMS: 2,99,2,99\r",
    "OK\r\n",
	  // AT+CMGD=1
    "OK\r\n",

	  // AT+CPMS="ME"
    "+CPMS: 1,99,1,99\r",
    "OK\r\n",
	  // AT+CMGR=2
    "+CMGR: 4,1,\"\",23\r",
    "07918444483252F0040B918446445078F700007121320144744004D4F29C0E\r",
    "OK\r\n",
	  // AT+CPMS="ME","ME"
    "+CPMS: 1,99,1,99\r",
    "OK\r\n",
	  // AT+CMGD=1
    "OK\r\n",

	  // Sentinel
    "ERROR\r\n"
  };
  SET_RESPONSES(responses);

  error = SMSD_ReadConfig("smsd.cfg", config, TRUE);
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
