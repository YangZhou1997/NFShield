# !/bin/bash

./hexembed ../data/ac.raw > ../data_embed/ac.h
./hexembed ../data/acl_fw_hashmap_dleft.raw > ../data_embed/acl_fw_hashmap_dleft.h
./hexembed ../data/ictf2010_100kflow_2mseq.dat > ../data_embed/ictf2010_100kflow_2mseq.h 
./hexembed ../data/ictf2010_100kflow.dat > ../data_embed/ictf2010_100kflow.h 
./hexembed ../data/sentense.rules > ../data_embed/sentense.rules.h