# NFs running on FireSim-simulated riscv quad-core rocket processor

## Install git LFS to get the data files (if you has not installed)
```
# on ubuntu
curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | sudo bash
sudo apt-get install git-lfs
git lfs install
# inside rep
git lfs pull
```
```
# on centos
sudo yum install git-lfs
```
For the other OS, please refer to [this page](https://github.com/git-lfs/git-lfs/wiki/Installation). 

Note that your git mush have version 2.3+ (checking by `git --version`) to let git-lfs work. 

## Build embedded .c data files
We embed some necessary data into .c files, so bare-metal binaries can directly include (via `extern`) and load them without file IO. 
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
make CONFIG='-DNF_STRINGS=\"l2_fwd\"'
```
This will build a `nftop.riscv` binary that runs l2_fwd on the first core (while the rest three cores are busy waiting), and copy it with necessary `.ini`, and `.json` to `$HOME/firesim/deploy/workloads`. You can then go there to start firesim simulation. 

You can change `DNF_STRINGS` to other NF to test new NFs: `acl_fw`, `dpi`, `lpm`, `maglev`, `monitoring`, `nat_tcp_v4`, `mem_test`. 

Currently, due to the lack of IO virtualization, we cannnot let multiple NFs efficiently share the IceNIC. Once having IO virtualization, we can build a `nftop.riscv` that runs different NFs on four cores by `make CONFIG='-DNF_STRINGS=\"l2_fwd:lpm:maglev:acl_fw\"'`.

A fresh make would take ~10min, in order to build the necessary embedded data from `data_embed/*.c`. After the fresh make, these embedded data will be stored as `*.o` and reused in the following make. 

Note that before testing new NFs by remaking with a new `CONFIG`, you should `make clean` to clear old binaries (eg, nftop.riscv, syscalls.o), but it will **not** remove *.o embedded data (which rarely gets changed). 

## Run firesim simulation
```
cd $HOME/firesim/ && source sourceme-f1-manager.sh && firesim managerinit
cd $HOME/firesim/deploy
firesim launchrunfarm -c workloads/nftop.ini
firesim infrasetup -c workloads/nftop.ini
firesim runworkload -c workloads/nftop.ini
firesim terminaterunfarm -c workloads/nftop.ini
```

## Get simulation results
You can either check the switchlog in firesim manager instance after simulation ends, or login to the switch simulation instance, and `cat cat switch_slot_0/switchlog`, where you can see the packet processing rate and bandwidth. 