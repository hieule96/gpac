#include "RTP_send.h"
#include "RTP_packetizer.h"
#include "RTP_transmitVideo.h"
#include <gpac/isomedia.h>
#include <gpac/constants.h>
#include <unistd.h>
#include "timing.h"

enum {
	RTP_Online=0,
	RTP_Offline=1
} SimulationMode;


// structure of option selected
typedef struct RTP_option{
	uint8_t s_mode;
	
} RTP_option;



RTP_packet* initRTPContext(GF_RTPChannel *p_chan, GF_RTPHeader *p_hdr){
    GF_Err e;
    RTP_packet *rtppacket = (RTP_packet *)gf_malloc(sizeof(RTP_packet));
    rtppacket->chan = p_chan;
    rtppacket->hdr = p_hdr;
	rtppacket->extension = gf_malloc(sizeof(RTP_packetExt));
    ((RTP_packetExt*) rtppacket->extension)->i = 0;
    ((RTP_packetExt*) rtppacket->extension)->lastTS = 0;
    return rtppacket;
}
int main(const int argc, const char** argv){
    GF_Err e;
    uint32_t mtu_size = 1492;
	int debug = 0;
   	char sdp_fmt[5000];
	u32 track_id = 1;
   	GF_ISOFile *movie;
	u64 missing_bytes;

	char *pck;
	u32 pck_size;
	Bool disposable;
	Bool repeated;
	u32 trans_ts;
	u32 sample_num;
	char *ip;
	u32 port;
  	GF_HEVCConfig *hevccfg = 0;
	GF_BitStream *bs;
	char *mp4_filename;
	u8 mode;
	if (argc < 2) {
		fprintf(stderr, "Usage: [-f/t] file mode or transmission mode \n");
		fprintf(stderr, "-i ipaddr \n");
		fprintf(stderr, "-p port \n");
		fprintf(stderr, "-m video track id \n");
		fprintf(stderr, "-n mp4 file name \n");
		fprintf(stderr, "input: %s\n", argv[0]);
		exit(EXIT_FAILURE);	
		return 1;
	}
    int opt;
	while ((opt = getopt(argc, argv, "fti:p:m:n:")) != -1) {
        switch (opt) {
			case 'f': mode = RTP_Offline; break;
			case 't': mode = RTP_Online; break;
			case 'i': 
				ip = optarg; 
				fprintf(stdout, "ip: %s\n", optarg);
				break;
			case 'p': port = atoi(optarg); 
				fprintf(stdout, "port: %s\n", optarg);
			break;
			case 'm': track_id = atoi(optarg); 
				fprintf(stdout, "track id: %s\n", optarg);
				break;
			case 'n': mp4_filename = optarg; 
				fprintf(stdout, "mp4 file name: %s\n", optarg);
				break;
			case '?':
				if (optopt == 'i' || optopt == 'p'|| optopt == 'm') {
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				}
				else if (isprint(optopt)) {
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				}
				else {
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				}
				return EXIT_FAILURE;
        default:
            fprintf(stderr, "Usage: [-f/t] file mode or transmission mode \n");
            fprintf(stderr, "-i ipaddr \n");
			fprintf(stderr, "-p port \n");
			fprintf(stderr, "-m video track id \n");
			fprintf(stderr, "-n mp4 file name \n");
			fprintf(stderr, "input: %s\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    // Open the hinted video file
	e = gf_isom_open_progressive(mp4_filename, 0, 0,GF_TRUE,&movie, &missing_bytes);
    // Initialize the GPAC framework
    gf_sys_init(GF_MemTrackerNone,NULL);

    // RTP package
    RTP_packet *rtppacket;
    // RTP channel
	GF_RTPChannel * chan;
    // Initialize the RTP channel
	if (mode == RTP_Online) {
		e = Evaltool_InitRTP(&chan, ip, port, mtu_size);
		if (e != 0) {
			fprintf(stderr, "Cannot initialize RTP output (error: %d)\n", e);
			exit(1);
		}
		// Initialize the RTP packet Context
		rtppacket = initRTPContext(chan, NULL);
		e = EvalTool_InitRTPPacketizer(rtppacket, sdp_fmt, mtu_size);
		if (e != 0) {
			fprintf(stderr, "Cannot initialize RTP Packetizer (error: %d)\n", e);
			exit(1);
		}
	}
    // A loading buffer is used to store the data to be sent, depend on the MTU size
    // A big buffer can be split off into several RTP packets
    // An timestamp is needed to add into the hinted file to regulate the bitrate and also simulate real FPS.
	e = gf_isom_reset_hint_reader(movie, track_id+1,1,0,0,0);
    if (e != 0) {
		fprintf(stderr, "gf_isom_reset_hint_reader(error: %d)\n", e);
		exit(1);
	}
	u32 track_num = gf_isom_get_track_by_id(movie, track_id);
	// Get Config Parameter for H265
	hevccfg = gf_isom_hevc_config_get(movie, track_num, 1);
	if (hevccfg) {
		u32 idx;
		u8 nalh_size;
		nalh_size = hevccfg->nal_unit_size;
		for (idx=0; idx<gf_list_count(hevccfg->param_array); idx++) {
			GF_NALUFFParamArray *ar = gf_list_get(hevccfg->param_array, idx);
			if (ar->type==GF_HEVC_NALU_SEQ_PARAM) {
				fprintf(stdout, "HEVC SPS\n");
				for (int i=0;i<gf_list_count(ar->nalus);i++){
					GF_NALUFFParam *slc=gf_list_get(ar->nalus, i);
					printf("%d\n", slc->size);
					if (mode == RTP_Online) {
			        	EvalTool_RTPPacketizerProcess(rtppacket, slc->data, slc->size, 0,1);
					}
				}
			} else if (ar->type==GF_HEVC_NALU_PIC_PARAM) {
				fprintf(stdout, "HEVC PPS\n");
				for (int i=0;i<gf_list_count(ar->nalus);i++){
					GF_NALUFFParam *slc=gf_list_get(ar->nalus, i);
					printf("%d\n", slc->size);
					if (mode == RTP_Online) {
			        	EvalTool_RTPPacketizerProcess(rtppacket, slc->data, slc->size, 0,1);
					}
				}
			} else if (ar->type==GF_HEVC_NALU_VID_PARAM) {
				fprintf(stdout, "HEVC VPS\n");
				for (int i=0;i<gf_list_count(ar->nalus);i++){
					GF_NALUFFParam *slc=gf_list_get(ar->nalus, i);
					printf("%d\n", slc->size);
					if (mode == RTP_Online) {
			        	EvalTool_RTPPacketizerProcess(rtppacket, slc->data, slc->size, 0,1);
					}				}
			} else {
				fprintf(stdout, "HEVCUnknownPS", "hvcC", 0);
			}
		}
	}
	//Check if sample_num is modified
	// int old_sample_num = 1;
	// if mode is RTP Offline, open file trace.txt to store the trace of the RTP package
	FILE *tracefile;

	if (mode == RTP_Offline) {
		tracefile = fopen("trace.txt", "w");
		if (tracefile == NULL) {
			fprintf(stderr, "Cannot open trace.txt\n");
			exit(1);
		}
	} 
	u32 id_seg = 1;
    get_cpu_freq();
	starttimer();
    while (1){
        e = gf_isom_next_hint_packet(movie, track_id+1, &pck, &pck_size, &disposable,&repeated,&trans_ts,&sample_num);
		if (e != GF_OK) break;
		fprintf(tracefile,"%d\tI\t%d\t%d\n",id_seg, (int) curtime(),pck_size);
		if (mode == RTP_Online) EvalTool_RTPPacketizerProcess(rtppacket, pck, pck_size, trans_ts,0);
		id_seg+=1;
        gf_sleep(1);
    }
	if (mode == RTP_Online) {
    	EvalTool_CloseRTPPacketizer(rtppacket);
    	gf_free(rtppacket->extension);
    	gf_free(rtppacket);
	}
  	gf_sys_close();
    return EXIT_SUCCESS;
}