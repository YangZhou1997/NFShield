# !/bin/bash

make clean && make
./hexembed ../data/ac.raw > ../data_embed/ac.c
./hexembed ../data/acl_fw_hashmap_dleft.raw > ../data_embed/acl_fw_hashmap_dleft.c
./hexembed ../data/ictf2010_100kflow_2mseq.dat > ../data_embed/ictf2010_100kflow_2mseq.c
./hexembed ../data/ictf2010_100kflow.dat > ../data_embed/ictf2010_100kflow.c
./hexembed ../data/sentense.rules > ../data_embed/sentense.rules.c