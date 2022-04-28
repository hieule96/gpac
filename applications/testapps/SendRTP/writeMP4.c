#include <stdio.h>
#include <stdlib.h>
#include <gpac/internal/isomedia_dev.h>
#include <gpac/media_tools.h>
#include <gpac/constants.h>
#include "writeMP4.h"

static void write_sample(GF_ISOSample *smp, GF_HEVCConfig *hevccfg, GF_BitStream *bs)
{
  if (!hevccfg) {
    gf_bs_write_data(bs, smp->data, smp->dataLength);
  } else {
    u32 j, nal_size, remain = smp->dataLength;
    u8 *ptr = smp->data;
    while (remain) {
      nal_size = 0;
      for (j = 0; j < hevccfg->nal_unit_size; j++) {
        nal_size |= *ptr;
        if (j + 1 < hevccfg->nal_unit_size) nal_size <<= 8;
        remain--;
        ptr++;
      }
      gf_bs_write_u32(bs, 1);
      if (nal_size > smp->dataLength || nal_size == 0) {
        gf_bs_write_data(bs, ptr, remain);
        remain = 0;
      } else {
        gf_bs_write_data(bs, ptr, nal_size);
        ptr += nal_size;
        remain -= nal_size;
      }
    }
  }
}


int writeHEVC(GF_ISOFile *seq, int hint_track_num, int ref_track_num,char *outputfilename,data_t *data){
    GF_HEVCConfig *hevccfg = NULL;
    // Bitstream structure
    GF_BitStream *bs = NULL;
    GF_HintSample *hint_smp,*bs_tmp=NULL;
    FILE *fout;
    if (0 == (fout = fopen(outputfilename, "wb"))){
        fprintf(stderr, "Cannot open output file %s\n", outputfilename);
        return GF_IO_ERR;
    }
    bs = gf_bs_from_file(fout, GF_BITSTREAM_WRITE);

    // Write the HEVC config first
	hevccfg = gf_isom_hevc_config_get(seq, ref_track_num, 1);
    if (hevccfg) {
        u32 idx;
		u8 nalh_size;
		nalh_size = hevccfg->nal_unit_size;
		for (idx=0; idx<gf_list_count(hevccfg->param_array); idx++) {
			GF_NALUFFParamArray *ar = gf_list_get(hevccfg->param_array, idx);
			if (ar->type==GF_HEVC_NALU_SEQ_PARAM) {
				fprintf(stdout,"HEVC SPS: ");
				for (int i=0;i<gf_list_count(ar->nalus);i++){
					GF_NALUFFParam *slc=gf_list_get(ar->nalus, i);
					printf("%d\n", slc->size);
                    gf_bs_write_u32(bs, 1);
                    gf_bs_write_data(bs,slc->data,slc->size);
				}
			} else if (ar->type==GF_HEVC_NALU_PIC_PARAM) {
				fprintf(stdout,"HEVC PPS: ");
				for (int i=0;i<gf_list_count(ar->nalus);i++){
					GF_NALUFFParam *slc=gf_list_get(ar->nalus, i);
					printf("%d\n", slc->size);
                    gf_bs_write_u32(bs, 1);
                    gf_bs_write_data(bs,slc->data,slc->size);
				}
			} else if (ar->type==GF_HEVC_NALU_VID_PARAM) {
				fprintf(stdout,"HEVC VPS: ");
				for (int i=0;i<gf_list_count(ar->nalus);i++){
					GF_NALUFFParam *slc=gf_list_get(ar->nalus, i);
					printf("%d\n", slc->size);
                    gf_bs_write_u32(bs, 1);
                    gf_bs_write_data(bs,slc->data,slc->size);
                    }
            } 
            else {
				fprintf(stdout, "HEVCUnknownPS", "hvcC", 0);
			}
		}
    }
    // Write the HEVC frames
    // get number of frames
    u32 di,l,j,k,i = 0;
    GF_ISOSample *smp,*tmp;
    GF_RTPPacket *pck = NULL;
    u32 count=0;
    u32 last_smp = 0;
    u32 packets = 0;
    u32 HEVC_samples = gf_isom_get_sample_count(seq, hint_track_num);
	gf_isom_set_nalu_extract_mode(seq,ref_track_num , GF_ISOM_NALU_EXTRACT_INSPECT);

    for (l = 0, i = 0; i < HEVC_samples; i++) {
        smp = gf_isom_get_sample(seq, hint_track_num, i+1, &di);
        if (smp) {
            hint_smp = gf_isom_hint_sample_new(GF_ISOM_BOX_TYPE_RTP_STSD);
            bs_tmp = gf_bs_new(smp->data, smp->dataLength, GF_BITSTREAM_READ);
            gf_isom_hint_sample_read(hint_smp, bs_tmp, smp->dataLength);
            gf_bs_del(bs_tmp);
            packets = gf_list_count(hint_smp->packetTable);
            for (j = 0; j < packets; j++, l++) {
                pck = gf_list_get(hint_smp->packetTable, j);
                count = gf_list_count(pck->DataTable);
                for (k = 0; k < count; k++) {
                GF_GenericDTE *dte = gf_list_get(pck->DataTable, k);
                switch (dte->source) {
                    /*empty*/
		            case 0:
			            break;
                    case 1:
                    	//gf_bs_write_data(bs, ((GF_ImmediateDTE *)dte)->data, ((GF_ImmediateDTE *)dte)->dataLength);
			            break;
                    case 2:
                    {
                        // The data is raw without any header
                        GF_SampleDTE *sdte = (GF_SampleDTE *)dte;
                        if (sdte->sampleNumber != last_smp) {
                            if (last_smp != 0) {
                                write_sample(tmp,hevccfg,bs);
                                gf_isom_sample_del(&tmp);
                                //fprintf(stdout,"%d,%d\n",last_smp);
                            }
                            if (0 == (tmp = gf_isom_get_sample(seq, ref_track_num, sdte->sampleNumber, 0))){
                                fprintf(stderr, "Cannot get sample %d\n", sdte->sampleNumber);
                                return GF_IO_ERR;
                            }
                            last_smp = sdte->sampleNumber;
                        }
                        if (data[l].info.lost) {
                            memset(tmp->data + sdte->byteOffset,0, sdte->dataLength);
                            fprintf(stdout,"Lost,%d,%d,%d\n",last_smp,data[l].info.id,data[l].info.size);
                        }
                        else
                        {
                            fprintf(stdout,"Received,%d,%d,%d,%d\n",last_smp,data[l].info.id,data[l].info.size,data[l].info.jitter);
                        }
                    }
                    case 3:
                        break;
                }
            }
            }
        }
    }
    if (tmp) gf_bs_write_data(bs, tmp->data, tmp->dataLength);
    gf_isom_sample_del(&tmp);
    if (hevccfg) gf_odf_hevc_cfg_del(hevccfg);
    gf_bs_del(bs);
    fclose(fout);
    return GF_OK;
}

int writeMP4(GF_ISOFile *seq, int hint_track_num, int ref_track_num,const char *outputfilename, data_t *data) {
    GF_ISOSample *smp = 0, *tmp = 0;
    GF_HintSample *hint_smp;
    GF_InitialObjectDescriptor *iod = 0;
    GF_RTPPacket *pck;
    GF_BitStream *bs_tmp;
    GF_ISOFile *fo;
    u32 total_size = 0;
    u32 i, j, k, l, count, di, samples, packets, last_smp = 0;
    u32 newTk, descIndex, trackID = gf_isom_get_track_id(seq, ref_track_num);
    if (0 == (fo = gf_isom_open(outputfilename, GF_ISOM_WRITE_EDIT, 0))) {
        fprintf(stderr, "Cannot open output file %s\n", outputfilename);
        return 1;
    }
    newTk = gf_isom_new_track(fo, trackID, gf_isom_get_media_type(seq, ref_track_num), gf_isom_get_media_timescale(seq, ref_track_num));
    gf_isom_set_track_enabled(fo, newTk, 1);
    gf_isom_clone_sample_description(fo, newTk, seq, ref_track_num, 1, 0, 0, &descIndex);
    samples = gf_isom_get_sample_count(seq,hint_track_num);
    for (l = 0, i = 0; i < samples; i++) {
        if (0 == (smp = gf_isom_get_sample(seq,hint_track_num,i + 1, &di))){
            fprintf(stderr, "Cannot get sample %d\n", i);
        }
        hint_smp = gf_isom_hint_sample_new(GF_ISOM_BOX_TYPE_RTP_STSD);
        bs_tmp = gf_bs_new(smp->data, smp->dataLength, GF_BITSTREAM_READ);
        gf_isom_hint_sample_read(hint_smp, bs_tmp, smp->dataLength);
        gf_bs_del(bs_tmp);
        packets = gf_list_count(hint_smp->packetTable);
        for (j = 0; j < packets; j++, l++) {
            pck = gf_list_get(hint_smp->packetTable, j);
            count = gf_list_count(pck->DataTable);
            for (k = 0; k < count; k++) {
                GF_GenericDTE *dte = gf_list_get(pck->DataTable, k);
                if (dte->source == 2) {
                    GF_SampleDTE *sdte = (GF_SampleDTE *)dte;
                    // Write the modified data block to the stream
                    if (sdte->sampleNumber != last_smp) {
                        if (last_smp != 0) {
                            if (GF_OK != gf_isom_add_sample(fo, newTk, descIndex, tmp)){
                                fprintf(stderr, "Cannot add sample %d\n", last_smp);
                                exit(1);
                            }
                            //fprintf(stdout,"MP4 block %d l:%d \n",sdte->sampleNumber,smp->dataLength);
                            total_size+= smp->dataLength;
                            gf_isom_sample_del(&tmp);
                        }
                        if (0 == (tmp = gf_isom_get_sample(seq, ref_track_num, sdte->sampleNumber, 0)))
                        {
                            fprintf(stderr, "Cannot get sample from data %d\n", sdte->sampleNumber);
                        }
                        last_smp = sdte->sampleNumber;
                    }
                    //TODO if some loss occcured, delete the data in the tempory buffer
                    if (data[l].info.lost) {
                        memset(tmp->data + sdte->byteOffset,0, sdte->dataLength);
                    }
                }
            }
        }
        gf_isom_sample_del(&smp);
    }
    if (iod) gf_odf_desc_del((GF_Descriptor *)iod);
    if (GF_OK != gf_isom_close(fo))
    {
        fprintf(stderr, "Cannot close file %s\n", outputfilename);
        exit(1);
    }
    fprintf(stdout,"Total size %d\n",total_size);

}