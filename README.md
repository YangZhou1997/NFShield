# NFs and pktgen running on FireSim-simulated riscv64 buildroot SMP linux

## Install git LFS to get the data files (if you has not installed)
```
curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | sudo bash
sudo apt-get install git-lfs
git lfs install
# inside rep
git lfs pull
```
For the other OS, please refer to [this page](https://github.com/git-lfs/git-lfs/wiki/Installation). 


## Initing a new ubuntu VM
```
wget -c https://releases.hashicorp.com/vagrant/2.0.3/vagrant_2.0.3_x86_64.deb
sudo dpkg -i vagrant_2.0.3_x86_64.deb

sudo apt-get remove --purge virtualbox-6.0
wget https://download.virtualbox.org/virtualbox/5.2.44/virtualbox-5.2_5.2.44-139111~Ubuntu~bionic_amd64.deb
sudo dpkg -i virtualbox-5.2_5.2.44-139111~Ubuntu~bionic_amd64.deb

cp Vagrantfile ~/
vagrant plugin install vagrant-disksize
vagrant up --provider=virtualbox
```

## Prepare riscv64 toolchain and linux image: [blog](https://www.embecosm.com/2018/09/19/adding-risc-v-64-bit-support-to-buildroot/.)
1.  Configure buildroot
    ```
    cd ~
    git clone https://github.com/riscv/riscv-buildroot
    cd riscv-buildroot
    git checkout riscv-start
    make qemu_riscv64_virt_defconfig
    ```

2. edit `.config` to increase disk size
    ```
    BR2_TARGET_ROOTFS_EXT2_SIZE="1G"
    ```

3. make 
    ```
    make sdk -j9 && make -j9
    ```

## Prepare riscv64 NF and pktgen binaries. 
```
source riscv_build_env.sh
make
```

## Start nftop in qemu with 4 NFs (cores)
Make sure that you have 4 cores in your NF server. 
```
bash run_qemu.sh 4
>> root
cd NF-GEM5 && ./nftop -n lpm -n dpi -n monitoring -n acl_fw -d DE:AD:BE:EF:B7:90
```

## Start pktgen in another qemu with 8 cores
Make sure that you have 8 cores in your pktgen server. When using less number of cores, the pktgen performance will decrease, and sometimes deadlock. 
**In order to get correct performance number, you should wait utill all NFs init done.**
```
bash run_qemu2.sh 8
>> root
cd NF-GEM5 && ./pktgen -n 4 -d DE:AD:BE:EF:7F:45
```

## Testing raw socket sending rate (64B)
```
./testRawSendingRate_64B eth0 DE:AD:BE:EF:7F:45
```

## Testing raw socket sending rate (real traffic trace)
```
./testRawSendingRate_trace eth0 DE:AD:BE:EF:7F:45
```

## Testing raw NF processing rate (real traffic trace)
```
./testRawNFRate_trace eth0 DE:AD:BE:EF:7F:45
```

## Notes
There is still rare cases that pktgen deadlock happens (may be caused by packet reordering/drops in the network link) when using 8 cores -- TO BE FIXED. 
When it happens, normally you just need to ctrl+c the pktgen, and rerun, then the deallock should disappear. 
Okay, it turns out to be the memory ordering issue, adding barrier() fixes. -- no, it cannot fix
Okay, it seems using large MAX_UNACK_WINDOW can fix it. 

TODO: 
cpu stats to measure the cpu utilization. -- test in VM
    not sure if kernel overhead is high?? 