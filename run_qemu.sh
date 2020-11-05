# !/bin/bash

source ./riscv_build_env.sh
# cp -r ~/NF-GEM5/ ~/riscv-buildroot/output/target/root/
rsync -rv --exclude=.git ~/NF-GEM5/ ~/riscv-buildroot/output/target/root/NF-GEM5
cd ~/riscv-buildroot/
make -j8
qemu-system-riscv64 -M virt -kernel output/images/bbl -append "root=/dev/vda ro console=ttyS0" -drive file=output/images/rootfs.ext2,format=raw,id=hd0 -device virtio-blk-device,drive=hd0 -netdev user,id=net0 -device virtio-net-device,netdev=net0 -nographic -m 1G