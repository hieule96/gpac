#include "writeMP4.h"
#include <string.h>
#include <unistd.h>

int ReadDumpPacket(char *f_udpdump,data_t *data)
{
  FILE *f_dump = 0;
  char c = 0;
  u32 line = 0;
  char buf[1024];
  u32 count=0;
  u32 buffer_size = 0;
  if ((f_dump = fopen(f_udpdump, "r")) == 0) {
    fprintf(stderr, "error opening %s\n", f_udpdump);
    exit(EXIT_FAILURE);
    return -1;
  }
  line = 0;
  // parse csv file and extract elements
  // Scan sender dump file
  // read each line in the file and save it in a char buffer buf
  while (fgets(buf,1024,f_dump)){
    const char *token = strtok(buf,",");
    if (token&&isdigit(token[0])){
      data[count].info.id = atoi(token);
    }
    //fprintf(stdout,"%s ",token);
    token = strtok(NULL,",");
    //fprintf(stdout,"%s ",token);
    
    token = strtok(NULL,",");
    if (token&&isdigit(token[0])){
      data[count].info.size = atoi(token);
      //fprintf(stdout,"%s ",token);
    }
    token = strtok(NULL,",");
    if (token&&isdigit(token[0])){
      //fprintf(stdout,"%s",token);
      data[count].info.t1 = atoi(token);
    }
    data[count].info.lost = true;
    count++;
  }

  if (f_dump) fclose(f_dump);
  return 0;
}

int UpdateRecvDumpPacket(char *f_udpdump,data_t *data){
  FILE *f_dump = 0;
  char buf[1024];
  if ((f_dump = fopen(f_udpdump, "r")) == 0) {
    fprintf(stderr, "error opening %s\n", f_udpdump);
    exit(EXIT_FAILURE);
    return -1;
  }
  u32 id = 0, size = 0, trans_ts = 0;
  while (fgets(buf,1024,f_dump)) {
    const char *token = strtok(buf,",");
    if (token&&isdigit(token[0])){
      id = atoi(token);
    }
    //fprintf(stdout,"%s ",token);
    token = strtok(NULL,",");
    //fprintf(stdout,"%s ",token);
    
    token = strtok(NULL,",");
    if (token&&isdigit(token[0])){
      size = atoi(token);
      //fprintf(stdout,"%s ",token);
    }
    token = strtok(NULL,",");
    if (token&&isdigit(token[0])){
      //fprintf(stdout,"%s",token);
      trans_ts= atoi(token);
    }


    data[id].info.size = size;
    data[id].info.t2 = trans_ts;
    data[id].info.jitter = data[id].info.t2 - data[id].info.t1;
    data[id].info.lost = false;
  }
  if (f_dump) fclose(f_dump);
}

int main(const int argc, const char** argv){
  GF_ISOFile *fi;
  size_t buffsize = 0;
  char *fileName_s=NULL;
  char *fileName_o=NULL;
  char *filename_tx=NULL;
  char *filename_rx=NULL;
  u32 i, tracks, samples = 0, ref_track = 1, hint_track = 2, type, sub;
  if (argc != 11) {
    fprintf(stderr, "Usage: -i inputfile.mp4 -o outputname -t txfile.txt -r rxfile.txt\n");
		fprintf(stderr, "input: %s %d\n", argv[0], argc);
		exit(EXIT_FAILURE);	
		return 1;
	}
  int opt;
	while ((opt = getopt(argc, argv, "i:o:t:r:s:")) != -1) {
    switch (opt) {
      case 'i':  
        fileName_s = optarg;
        fprintf(stdout, "Input File: %s\n", optarg);
        break;
      case 'o':
        fileName_o = optarg;
        fprintf(stdout, "Output File: %s\n", optarg);
        break;  
      break;
      case 't':
        filename_tx = optarg;
        fprintf(stdout, "Transmission File: %s\n", optarg);
        break;
      case 'r':
        filename_rx = optarg;
        fprintf(stdout, "Received File: %s\n", optarg);
        break;
      case 's':
        buffsize = atoi(optarg);
        fprintf(stdout, "Packet Buff size: %d\n", buffsize);
        break;
      case '?':
        if (optopt == 'i' || optopt == 'o'|| optopt == 's'|| optopt == 'r') {
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
            fprintf(stderr, "Usage: -i inputfile.mp4 -o outputname -t txfile.txt -r rxfile.txt\n");
            exit(EXIT_FAILURE);
        }
    }
  data_t *D_packet= malloc(buffsize*sizeof(data_t));
  if (0 == (fi = gf_isom_open(fileName_s, GF_ISOM_OPEN_READ, 0))) {
    fprintf(stderr, "Couldn't open file %s: %s\n", fileName_s, gf_error_to_string(gf_isom_last_error(0)));
    return EXIT_FAILURE;
  }
  ReadDumpPacket(filename_tx,D_packet);
  UpdateRecvDumpPacket(filename_rx,D_packet);
  char namehevc[20];
  char namemp4[20];
  snprintf(namehevc,sizeof(namehevc),"%s.hevc",fileName_o);
  snprintf(namemp4,sizeof(namemp4),"%s.mp4",fileName_o);
  writeMP4(fi, hint_track,ref_track,namemp4,D_packet);
  writeHEVC(fi, hint_track,ref_track,namehevc,D_packet);

  gf_isom_close(fi);
  if (D_packet) free(D_packet);
  return EXIT_SUCCESS;
}