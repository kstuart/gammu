#include "test_helper.h"
#include "../../libgammu/gsmstate.h"
#include <string.h>

/* "AT+CNMI=?"
 * "+CNMI: (0-2),(0-3),(0,2),(0-2),(0,1)"
 * "OK"
 */

GSM_Error ATGEN_ReplyGetCNMIMode(GSM_Protocol_Message *msg, GSM_StateMachine *s);
    
int main(void)
{
    GSM_Error error;
    GSM_StateMachine *s = setup_state_machine();
    GSM_Phone_ATGENData *Priv = setup_at_engine(s);
    GSM_Protocol_Message msg;
    const int RequestedParams[5] = {2,1,0,1,0};

    const char *event = "AT+CNMI=?\r+CNMI: (0-2),(0-3),(0,2),(0-2),(0,1)\rOK\r\n";

    s->Phone.Data.RequestID = ID_GetCNMIMode;
    memcpy(&s->CurrentConfig->CNMIParams, RequestedParams, sizeof(RequestedParams));
    msg.Length = strlen(event);
    msg.Buffer = (char*)event;
    msg.Type = 0;
    SplitLines(msg.Buffer, msg.Length, &Priv->Lines, "\x0D\x0A", 2, "\"", 1, TRUE);

    s->Phone.Data.RequestMsg = &msg;
    
    Priv->ReplyState = AT_Reply_OK;
    error = ATGEN_ReplyGetCNMIMode(&msg, s);
    test_result(error == ERR_NONE);

    test_result(Priv->CNMIMode == 2);
    test_result(Priv->CNMIProcedure == 1);
    test_result(Priv->CNMIBroadcastProcedure == 0);
    test_result(Priv->CNMIDeliverProcedure == 1);
    test_result(Priv->CNMIClearUnsolicitedResultCodes == 0);

    cleanup_state_machine(s);
}
