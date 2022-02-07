# !/bin/bash
source ~/chipyard/env.sh
cd ~/NF-GEM5 && make clean && make -j
# rsync -rv --exclude=.git --max-size=500m ~/NF-GEM5 ~/firesim/sw/firesim-software/example-workloads/example-fed/overlay/root/
rsync -rv --exclude=.git --max-size=500m ~/NF-GEM5 ~/firesim/sw/firesim-software/boards/firechip/base-workloads/fedora-base/overlay/root/

cd ~/chipyard-lnic/tests-icenic && make clean && make -j
rsync -rv --exclude=.git --max-size=500m ~/chipyard-lnic/tests-icenic ~/firesim/sw/firesim-software/boards/firechip/base-workloads/fedora-base/overlay/root/
