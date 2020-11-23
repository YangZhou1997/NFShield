#include "../utils/pkt-puller.h"

int main(){
    load_pkt("../data/ictf2010_100kflow.dat");
    int cnt = 0;

    FILE *f_gem5 = fopen("../data/ictf2010_2mpacket_zipf.dat", "w");
    uint64_t pkt_size = 0;
    while(cnt < 2*1024*1024){
        pkt_t* pkt = next_pkt(0);
        cnt ++;

        pkt_size += pkt->len;
		fwrite(&pkt->len, sizeof(uint16_t), 1, f_gem5);
        fwrite(pkt->content, 1, pkt->len, f_gem5);

        if(cnt % (1024*1024) == 0) {
            printf("cnt = %u, average packet size = %lf\n", cnt, pkt_size * 1.0 / cnt);
        }
    }
    fclose(f_gem5);
    return 0;
}