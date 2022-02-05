# !/bin/bash
source ~/chipyard/env.sh
make clean && make -j
rsync -rv --exclude=.git --max-size=500m ~/NF-GEM5 ~/firesim/sw/firesim-software/example-workloads/example-fed/overlay/root/
