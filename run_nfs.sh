# !/bin/bash
sysctl -w net.core.wmem_max=4194304
sysctl -w net.core.wmem_default=4194304
sysctl -w net.core.rmem_max=4194304
sysctl -w net.core.rmem_default=4194304

./nftop -n lpm -n dpi -n monitoring -n acl_fw -d DE:AD:BE:EF:B7:90