#include "RTP_packetizer.h"
#include "RTP_send.h"
#include <assert.h>


#include <gpac/ietf.h>
#include <gpac/mpeg4_odf.h>
#include <gpac/internal/ietf_dev.h>
#include <gpac/internal/media_dev.h>

#define MAX_PACKET_SIZE 2000

void OnNewPacket(void *cbk, GF_RTPHeader *header)
{
	((RTP_packet *)cbk)->formattedPacketLength = 0;
}

void OnPacketDone(void *cbk, GF_RTPHeader *header)
{
	RTP_packet *data = (RTP_packet *)cbk;
	dprintf(DEBUG_RTP_serv_packetizer, "RTP Packet done\n");
	Evaltool_sendRTP(data, (u8 *)data->formattedPacket, data->formattedPacketLength);
	((RTP_packet *)cbk)->formattedPacketLength = 0;
}

void OnData(void *cbk, u8 *data, u32 data_size, Bool is_head)
{
	memcpy(((RTP_packet *)cbk)->formattedPacket+((RTP_packet *)cbk)->formattedPacketLength, data, data_size);
	((RTP_packet *)cbk)->formattedPacketLength += data_size;
}

GF_Err EvalTool_InitRTPPacketizer(RTP_packet * data, char *sdp_fmt, unsigned short mtu_size)
{
	GP_RTPPacketizer *p;
	GF_SLConfig sl;
	memset(&sl, 0, sizeof(sl));

	sl.useTimestampsFlag = 1;
	sl.useRandomAccessPointFlag = 1;
	sl.timestampResolution = 1000;
	sl.AUSeqNumLength = 16;

	p = gf_rtp_builder_new(GF_RTP_PAYT_HEVC,
	                       NULL,
	                       GP_RTP_PCK_SIGNAL_RAP | GP_RTP_PCK_SIGNAL_AU_IDX,
	                       data,
	                       OnNewPacket,
	                       OnPacketDone,
	                       NULL,
	                       OnData);
	if (!p) {
		return GF_OUT_OF_MEM;
	}

	/* Mtu size - 20 = payload max size */
	mtu_size-=20;
	gf_rtp_builder_init(p, 96, mtu_size, 0, 3, 1, 1, 0, 0, 0, 0, 0, 0, NULL);
	gf_rtp_builder_format_sdp(p, "FU", sdp_fmt, NULL, 0);
	p->rtp_header.Version=2;
	p->rtp_header.SSRC=rand();
	data->hdr=& p->rtp_header;
	data->rtpBuilder=p;
	data->formattedPacket = (u8 *) gf_malloc(MAX_PACKET_SIZE);
	data->formattedPacketLength = 0;
	return GF_OK;
}

void EvalTool_CloseRTPPacketizer(RTP_packet *rtppacket)
{
	gf_free(rtppacket->formattedPacket);
	gf_rtp_builder_del(rtppacket->rtpBuilder);
}

GF_Err EvalTool_RTPPacketizerProcess(RTP_packet * data, u8 *au, u32 size, u64 ts,u8 IsAUEnd){
    assert( data );
	assert( au );
	/* We need to set a TS different every time */
	data->hdr->TimeStamp = (u32) gf_sys_clock();
	data->rtpBuilder->sl_header.compositionTimeStamp = (u32) gf_sys_clock();
	data->rtpBuilder->sl_header.randomAccessPointFlag = data->RAP;
  	if (data->SAUN_inc) data->rtpBuilder->sl_header.AU_sequenceNumber++;

 	/* reset input data config */   
	data->RAP=0;
	data->SAUN_inc=0;

    data->rtpBuilder->sl_header.paddingBits = 0;
	gf_rtp_builder_process(data->rtpBuilder, au, size, IsAUEnd, size, 0, 0);
	return GF_OK;
}