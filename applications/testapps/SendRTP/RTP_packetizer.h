#ifndef __RTP_PACKETIZER
#define __RTP_PACKETIZER

#include <gpac/ietf.h>
#include "debug.h"
typedef struct {
    GP_RTPPacketizer *rtpBuilder;
	GF_RTPChannel *chan;
	GF_RTPHeader *hdr;
    u8 * formattedPacket;
	int formattedPacketLength;

	void *extension;

	/* indication that the Access Unit is a RAP */
	int RAP;
	/* RAP counter */
	int RAPsent;
	/* indication that the Access Unit Sequence Number should be increased */
	int SAUN_inc;
} RTP_packet;

typedef struct{
	int i;
	int lastTS;
} RTP_packetExt;

GF_Err EvalTool_InitRTPPacketizer(RTP_packet *rtppacket, char *sdp_fmt, unsigned short mtu_size);
void EvalTool_CloseRTPPacketizer(RTP_packet *rtppacket);
GF_Err EvalTool_RTPPacketizerProcess(RTP_packet * data, u8 *au, u32 size, u64 ts,u8 IsAUEnd);
#endif