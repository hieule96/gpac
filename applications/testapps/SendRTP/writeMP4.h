#ifndef __writeMP4_h__
#define __writeMP4_h__
#include <gpac/isomedia.h>
#include <stdbool.h>

typedef struct {
  u32 id, size, time;
  u32 t1, t2, jitter;
  bool lost;
} dump_t;

typedef struct {
  dump_t info;
  unsigned long nP;
} data_t;
int writeHEVC(GF_ISOFile *seq, int hint_track_num, int ref_track_num,char *outputfilename,data_t *data);
int writeMP4(GF_ISOFile *seq, int hint_track_num, int ref_track_num,const char *outputfilename, data_t *data);
#endif