# NFs and pktgen running on FireSim-simulated riscv64 buildroot SMP linux

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

## Prepare risc64 NF and pktgen binaries. 
```
source riscv_build_env.sh
make
```

## Start nftop in qemu with 4 NFs (cores)
```
bash run_qemu.sh 4
>> root
cd NF-GEM5 && ./nftop -n lpm -n dpi -n monitoring -n acl_fw
```

## Start pktgen in another qemu with 8 cores
```
bash run_qemu2.sh 8
>> root
cd NF-GEM5 && ./pktgen -n 4
```

