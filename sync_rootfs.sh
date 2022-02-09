# !/bin/bash
source ~/chipyard/env.sh
cd ~/NF-GEM5 && make clean && make -j
cd ~/NF-GEM5/test_bw && make clean && make -j
# rsync -rv --exclude=.git --max-size=500m ~/NF-GEM5 ~/firesim/sw/firesim-software/example-workloads/example-fed/overlay/root/
rsync -rv --exclude=.git --max-size=500m ~/NF-GEM5 ~/firesim/sw/firesim-software/boards/firechip/base-workloads/fedora-base/overlay/root/
