#ifndef __RTP_SENDER
#define __RTP_SENDER
#include <gpac/ietf.h>
#include "RTP_packetizer.h"
GF_Err Evaltool_sendRTP(RTP_packet *data, u8 *payload, int payloadSize);
extern GF_Err Evaltool_InitRTP(GF_RTPChannel **chan, char *dest, int port, unsigned short mtu_size);
extern GF_Err Evaltool_CloseRTP(GF_RTPChannel *chan);

#endif