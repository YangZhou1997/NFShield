# NFs running on FireSim-simulated riscv quad-core rocket processor

## Install git LFS to get the data files (if you has not installed)
```
curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | sudo bash
sudo apt-get install git-lfs
git lfs install
# inside rep
git lfs pull
```
For the other OS, please refer to [this page](https://github.com/git-lfs/git-lfs/wiki/Installation). 

## Build embedded .h data files
We embed some necessary data into .h files, so bare-metal binaries can directly include and load them without file IO.  
```
cd data_prep && ./hexembed_run.sh && cd -
```
This may take ~5mins

## Buildinng riscv64 toolchain
```
git clone git@github.com:YangZhou1997/firesim.git
cd firesim && ./build-setup.sh fast
export PATH=$HOME/firesim/target-design/chipyard/riscv-tools-install/bin:$PATH
```
Enter `y` if prompting any confirmation questions.

## Prepare riscv64 NF binary
```
make CONFIG="-DNF_STRINGS=l2_fwd:l2_fwd:l2_fwd:l2_fwd"
```
This will build a `nftop.riscv` binary running l2_fwd on four cores, and copy it with necessary `.ini`, and `.json` to `$HOME/firesim/deploy/workloads`. You can then go there to start firesim simulation.

## Run firesim simulation
```
cd $HOME/firesim/ && source sourceme-f1-manager.sh && firesim managerinit
cd $HOME/firesim/deploy/workloads
firesim launchrunfarm -c nftop.ini
firesim infrasetup -c nftop.ini
firesim runworkload -c nftop.ini
firesim terminaterunfarm -c nftop.ini
```

## Get simulation results
TODO