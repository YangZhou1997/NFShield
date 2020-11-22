#include "../utils/pkt-puller.h"

int main(){
    int cnt = 0;
    FILE *f_seq = fopen("../data/ictf2010_100kflow_2mseq.dat", "w");
    uint64_t pkt_size = 0;
    while(cnt < 2*1024*1024){
        uint32_t zipf_r = popzipf(PKT_NUM, 1.1);
		fwrite(&zipf_r, sizeof(uint32_t), 1, f_seq);
        cnt ++;
    }
    fclose(f_seq);
    return 0;
}