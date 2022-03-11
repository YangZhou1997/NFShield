# !/bin/bash
sysctl -w net.core.wmem_max=4194304
sysctl -w net.core.wmem_default=4194304
sysctl -w net.core.rmem_max=4194304
sysctl -w net.core.rmem_default=4194304

./pktgen -n 4 -d DE:AD:BE:EF:7F:45