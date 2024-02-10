#include <stdio.h>
#include <stdlib.h>

#define VALUE_TYPE BOOL // hashset, bool

#include "./utils/common.h"
#include "./utils/dleft-hash.h"
#include "./utils/pkt-ops.h"
#include "./utils/pkt-puller.h"

// int(200000 / 0.875)
// make sure we use the same about of memory as netbricks.
#define HT_SIZE 228571

static dleft_meta_t ht_meta;
static dleft_meta_t ht_meta_cache;

#define ACL_RULE_NUM 1596
// #define ACL_RULE_NUM_USED 3192
#define ACL_RULE_NUM_USED 643


char * FW_RULES[ACL_RULE_NUM] = {"103.75.118.230/32", "104.194.215.118/32", "104.194.215.161/32", "104.194.215.57/32", "104.227.137.35/32", "107.172.141.10/32", "107.172.143.241/32", "107.172.2.128/32", "107.172.248.98/32", "107.173.140.104/32", "107.173.141.227/32", "107.173.34.151/32", "107.173.42.177/32", "107.175.62.231/32", "107.175.69.34/32", "107.181.175.122/32", "107.181.175.44/32", "107.181.175.69/32", "107.191.109.143/32", "108.174.56.159/32", "109.234.35.127/32", "110.142.247.110/32", "12.149.72.170/32", "12.162.84.2/32", "136.243.117.85/32", "136.243.142.131/32", "137.74.173.19/32", "137.74.211.228/32", "139.60.163.38/32", "139.60.163.39/32", "139.60.163.40/32", "142.44.157.144/32", "144.139.247.220/32", "144.217.12.34/32", "144.217.50.240/32", "144.217.50.241/32", "144.217.50.243/32", "146.185.219.31/32", "146.185.219.45/32", "146.185.219.56/32", "146.185.219.69/32", "148.103.7.35/32", "148.251.33.195/32", "149.202.225.160/32", "149.202.225.164/32", "151.237.93.131/32", "151.80.88.253/32", "159.0.130.149/32", "162.144.254.125/32", "162.244.32.130/32", "162.244.32.139/32", "162.244.32.226/32", "162.247.155.131/32", "164.132.138.130/32", "164.132.216.41/32", "164.132.216.63/32", "165.227.213.173/32", "168.235.102.16/32", "168.235.81.133/32", "172.245.19.134/32", "172.245.241.25/32", "173.175.79.89/32", "173.230.145.224/32", "175.100.138.82/32", "176.119.158.184/32", "177.230.108.144/32", "177.242.214.30/32", "177.246.193.139/32", "178.156.202.151/32", "178.156.202.69/32", "178.157.82.177/32", "178.157.82.60/32", "178.170.189.45/32", "178.63.50.54/32", "180.150.87.75/32", "181.129.30.82/32", "182.180.170.72/32", "183.82.100.135/32", "185.117.73.76/32", "185.117.75.101/32", "185.135.81.147/32", "185.135.81.95/32", "185.141.25.101/32", "185.141.25.116/32", "185.141.26.80/32", "185.172.129.109/32", "185.172.129.147/32", "185.172.129.150/32", "185.175.156.13/32", "185.180.197.38/32", "185.180.198.148/32", "185.183.97.133/32", "185.183.97.141/32", "185.186.77.248/32", "185.198.57.110/32", "185.202.174.72/32", "185.205.210.58/32", "185.206.145.100/32", "185.206.146.178/32", "185.224.132.65/32", "185.224.134.124/32", "185.242.84.174/32", "185.244.149.216/32", "185.244.149.74/32", "185.244.219.46/32", "185.246.64.77/32", "185.246.67.198/32", "185.246.67.63/32", "185.251.38.238/32", "185.251.38.244/32", "185.251.39.73/32", "185.252.144.213/32", "185.252.144.55/32", "185.43.6.250/32", "185.43.6.87/32", "185.61.148.203/32", "185.61.148.207/32", "185.61.148.65/32", "185.61.149.162/32", "185.62.103.137/32", "185.65.202.102/32", "185.66.14.248/32", "185.66.15.10/32", "185.82.200.7/32", "185.86.149.110/32", "185.86.151.115/32", "185.86.151.23/32", "185.90.61.7/32", "185.90.61.80/32", "185.94.252.102/32", "185.98.87.113/32", "186.103.199.252/32", "186.118.161.100/32", "186.4.234.27/32", "186.94.231.146/32", "187.188.166.192/32", "188.246.227.208/32", "188.68.210.159/32", "189.197.62.222/32", "189.253.39.50/32", "190.13.211.174/32", "190.146.128.35/32", "190.147.116.32/32", "190.183.58.155/32", "190.24.243.186/32", "190.3.183.19/32", "190.97.219.241/32", "191.101.251.141/32", "191.101.251.146/32", "191.92.69.115/32", "192.144.7.42/32", "192.155.90.90/32", "192.210.132.15/32", "192.227.204.230/32", "192.227.232.26/32", "192.243.101.211/32", "192.243.102.102/32", "192.243.103.153/32", "192.243.108.102/32", "192.243.108.137/32", "192.243.108.230/32", "192.3.83.168/32", "192.95.11.45/32", "192.99.255.37/32", "193.124.176.170/32", "193.169.54.12/32", "193.239.235.209/32", "193.37.212.106/32", "193.37.212.249/32", "193.37.212.81/32", "193.37.213.39/32", "193.46.83.9/32", "194.32.77.81/32", "194.36.191.21/32", "194.36.191.23/32", "194.67.217.125/32", "194.87.103.190/32", "194.88.246.242/32", "195.123.212.139/32", "195.123.212.230/32", "195.123.213.186/32", "195.123.213.36/32", "195.123.226.124/32", "195.123.227.224/32", "195.123.233.162/32", "195.123.233.99/32", "195.123.237.186/32", "195.123.238.152/32", "195.123.238.188/32", "195.123.240.126/32", "195.123.240.148/32", "195.123.246.177/32", "195.123.246.2/32", "195.123.246.41/32", "195.123.246.67/32", "195.123.246.69/32", "195.133.144.87/32", "195.133.196.102/32", "195.161.114.191/32", "195.161.62.25/32", "198.12.71.138/32", "198.12.71.171/32", "198.46.190.37/32", "198.61.207.174/32", "198.74.58.47/32", "198.8.91.37/32", "199.119.78.54/32", "200.124.245.125/32", "201.236.95.82/32", "201.239.154.191/32", "201.97.95.50/32", "206.214.220.81/32", "206.217.143.91/32", "206.248.110.184/32", "207.58.168.91/32", "208.78.100.202/32", "212.109.223.12/32", "212.22.77.5/32", "212.5.159.61/32", "212.80.216.202/32", "213.14.166.152/32", "213.192.1.170/32", "213.226.68.117/32", "213.226.68.75/32", "213.226.71.157/32", "216.240.36.142/32", "216.81.62.54/32", "217.107.34.77/32", "217.13.106.16/32", "217.13.106.246/32", "217.13.106.249/32", "217.86.203.2/32", "23.94.137.10/32", "23.94.137.179/32", "23.94.137.223/32", "23.94.184.45/32", "23.94.93.104/32", "23.94.93.106/32", "23.94.96.203/32", "23.95.242.213/32", "23.95.9.122/32", "23.95.9.156/32", "24.243.101.134/32", "2.59.40.240/32", "27.109.24.214/32", "31.184.255.100/32", "31.202.132.159/32", "31.202.132.95/32", "37.18.30.179/32", "37.18.30.55/32", "37.18.30.99/32", "37.44.212.120/32", "37.44.212.173/32", "37.44.212.182/32", "37.44.212.195/32", "37.44.212.196/32", "37.44.212.203/32", "37.44.212.86/32", "37.44.215.106/32", "37.44.215.107/32", "37.44.215.156/32", "37.44.215.169/32", "39.61.34.254/32", "41.202.77.180/32", "41.32.82.216/32", "43.229.62.186/32", "45.12.215.57/32", "45.15.253.25/32", "45.15.253.27/32", "45.15.253.61/32", "45.67.228.192/32", "45.67.228.231/32", "45.73.17.164/32", "45.8.229.101/32", "45.8.229.113/32", "46.30.41.155/32", "51.254.160.193/32", "51.254.69.233/32", "51.75.58.173/32", "51.83.52.180/32", "5.188.168.61/32", "5.188.41.101/32", "5.189.224.172/32", "5.196.192.222/32", "5.196.195.6/32", "5.196.247.4/32", "5.230.147.179/32", "5.253.63.106/32", "5.253.63.112/32", "5.253.63.119/32", "5.67.205.99/32", "58.171.215.214/32", "58.65.211.99/32", "5.9.167.178/32", "60.32.214.242/32", "60.48.253.12/32", "65.49.60.163/32", "66.70.218.38/32", "66.70.218.60/32", "69.251.12.43/32", "69.43.168.200/32", "69.43.168.215/32", "69.45.19.251/32", "71.244.60.231/32", "76.86.20.103/32", "77.220.64.57/32", "77.244.219.49/32", "78.100.187.118/32", "78.155.206.28/32", "78.155.206.85/32", "78.155.207.191/32", "78.186.5.109/32", "78.47.56.190/32", "79.137.119.212/32", "80.227.184.182/32", "80.82.115.164/32", "80.87.193.47/32", "80.87.199.224/32", "81.177.136.36/32", "81.177.140.60/32", "82.118.21.133/32", "82.118.21.139/32", "82.118.21.91/32", "82.118.22.87/32", "82.202.194.179/32", "82.202.194.182/32", "82.202.221.160/32", "82.211.30.202/32", "84.200.208.98/32", "85.104.59.244/32", "85.119.150.159/32", "85.143.218.104/32", "85.143.219.53/32", "85.143.223.89/32", "85.204.116.131/32", "85.204.116.132/32", "85.204.116.138/32", "85.204.116.151/32", "85.204.116.41/32", "85.204.116.45/32", "85.204.116.94/32", "85.204.116.95/32", "85.25.210.172/32", "85.25.33.71/32", "86.99.35.122/32", "87.106.145.51/32", "88.215.2.29/32", "88.97.26.73/32", "89.105.203.180/32", "89.186.26.179/32", "89.32.41.122/32", "89.32.41.126/32", "89.32.41.127/32", "89.32.41.164/32", "91.235.129.119/32", "91.235.129.166/32", "91.235.129.167/32", "91.235.129.212/32", "91.240.84.110/32", "91.240.84.166/32", "91.240.85.141/32", "91.240.85.19/32", "92.119.114.186/32", "92.38.171.12/32", "92.38.171.20/32", "93.189.42.220/32", "93.189.44.235/32", "93.189.47.93/32", "93.95.97.209/32", "93.95.97.68/32", "94.103.94.103/32", "94.140.125.143/32", "94.156.189.197/32", "95.181.179.96/32", "95.181.198.173/32", "95.46.8.206/32", "103.75.118.230/32", "104.194.215.118/32", "104.194.215.161/32", "104.194.215.57/32", "104.227.137.35/32", "107.172.141.10/32", "107.172.143.241/32", "107.172.2.128/32", "107.172.248.98/32", "107.173.140.104/32", "107.173.141.227/32", "107.173.34.151/32", "107.173.42.177/32", "107.175.62.231/32", "107.175.69.34/32", "107.181.175.122/32", "107.181.175.44/32", "107.181.175.69/32", "107.191.109.143/32", "108.174.56.159/32", "109.234.35.127/32", "110.142.247.110/32", "12.149.72.170/32", "12.162.84.2/32", "136.243.117.85/32", "136.243.142.131/32", "137.74.173.19/32", "137.74.211.228/32", "139.60.163.38/32", "139.60.163.39/32", "139.60.163.40/32", "142.44.157.144/32", "144.139.247.220/32", "144.217.12.34/32", "144.217.50.240/32", "144.217.50.241/32", "144.217.50.243/32", "146.185.219.31/32", "146.185.219.45/32", "146.185.219.56/32", "146.185.219.69/32", "148.103.7.35/32", "148.251.33.195/32", "149.202.225.160/32", "149.202.225.164/32", "151.237.93.131/32", "151.80.88.253/32", "159.0.130.149/32", "162.144.254.125/32", "162.244.32.130/32", "162.244.32.139/32", "162.244.32.226/32", "162.247.155.131/32", "164.132.138.130/32", "164.132.216.41/32", "164.132.216.63/32", "165.227.213.173/32", "168.235.102.16/32", "168.235.81.133/32", "172.245.19.134/32", "172.245.241.25/32", "173.175.79.89/32", "173.230.145.224/32", "175.100.138.82/32", "176.119.158.184/32", "177.230.108.144/32", "177.242.214.30/32", "177.246.193.139/32", "178.156.202.151/32", "178.156.202.69/32", "178.157.82.177/32", "178.157.82.60/32", "178.170.189.45/32", "178.63.50.54/32", "180.150.87.75/32", "181.129.30.82/32", "182.180.170.72/32", "183.82.100.135/32", "185.117.73.76/32", "185.117.75.101/32", "185.135.81.147/32", "185.135.81.95/32", "185.141.25.101/32", "185.141.25.116/32", "185.141.26.80/32", "185.172.129.109/32", "185.172.129.147/32", "185.172.129.150/32", "185.175.156.13/32", "185.180.197.38/32", "185.180.198.148/32", "185.183.97.133/32", "185.183.97.141/32", "185.186.77.248/32", "185.198.57.110/32", "185.202.174.72/32", "185.205.210.58/32", "185.206.145.100/32", "185.206.146.178/32", "185.224.132.65/32", "185.224.134.124/32", "185.242.84.174/32", "185.244.149.216/32", "185.244.149.74/32", "185.244.219.46/32", "185.246.64.77/32", "185.246.67.198/32", "185.246.67.63/32", "185.251.38.238/32", "185.251.38.244/32", "185.251.39.73/32", "185.252.144.213/32", "185.252.144.55/32", "185.43.6.250/32", "185.43.6.87/32", "185.61.148.203/32", "185.61.148.207/32", "185.61.148.65/32", "185.61.149.162/32", "185.62.103.137/32", "185.65.202.102/32", "185.66.14.248/32", "185.66.15.10/32", "185.82.200.7/32", "185.86.149.110/32", "185.86.151.115/32", "185.86.151.23/32", "185.90.61.7/32", "185.90.61.80/32", "185.94.252.102/32", "185.98.87.113/32", "186.103.199.252/32", "186.118.161.100/32", "186.4.234.27/32", "186.94.231.146/32", "187.188.166.192/32", "188.246.227.208/32", "188.68.210.159/32", "189.197.62.222/32", "189.253.39.50/32", "190.13.211.174/32", "190.146.128.35/32", "190.147.116.32/32", "190.183.58.155/32", "190.24.243.186/32", "190.3.183.19/32", "190.97.219.241/32", "191.101.251.141/32", "191.101.251.146/32", "191.92.69.115/32", "192.144.7.42/32", "192.155.90.90/32", "192.210.132.15/32", "192.227.204.230/32", "192.227.232.26/32", "192.243.101.211/32", "192.243.102.102/32", "192.243.103.153/32", "192.243.108.102/32", "192.243.108.137/32", "192.243.108.230/32", "192.3.83.168/32", "192.95.11.45/32", "192.99.255.37/32", "193.124.176.170/32", "193.169.54.12/32", "193.239.235.209/32", "193.37.212.106/32", "193.37.212.249/32", "193.37.212.81/32", "193.37.213.39/32", "193.46.83.9/32", "194.32.77.81/32", "194.36.191.21/32", "194.36.191.23/32", "194.67.217.125/32", "194.87.103.190/32", "194.88.246.242/32", "195.123.212.139/32", "195.123.212.230/32", "195.123.213.186/32", "195.123.213.36/32", "195.123.226.124/32", "195.123.227.224/32", "195.123.233.162/32", "195.123.233.99/32", "195.123.237.186/32", "195.123.238.152/32", "195.123.238.188/32", "195.123.240.126/32", "195.123.240.148/32", "195.123.246.177/32", "195.123.246.2/32", "195.123.246.41/32", "195.123.246.67/32", "195.123.246.69/32", "195.133.144.87/32", "195.133.196.102/32", "195.161.114.191/32", "195.161.62.25/32", "198.12.71.138/32", "198.12.71.171/32", "198.46.190.37/32", "198.61.207.174/32", "198.74.58.47/32", "198.8.91.37/32", "199.119.78.54/32", "200.124.245.125/32", "201.236.95.82/32", "201.239.154.191/32", "201.97.95.50/32", "206.214.220.81/32", "206.217.143.91/32", "206.248.110.184/32", "207.58.168.91/32", "208.78.100.202/32", "212.109.223.12/32", "212.22.77.5/32", "212.5.159.61/32", "212.80.216.202/32", "213.14.166.152/32", "213.192.1.170/32", "213.226.68.117/32", "213.226.68.75/32", "213.226.71.157/32", "216.240.36.142/32", "216.81.62.54/32", "217.107.34.77/32", "217.13.106.16/32", "217.13.106.246/32", "217.13.106.249/32", "217.86.203.2/32", "23.94.137.10/32", "23.94.137.179/32", "23.94.137.223/32", "23.94.184.45/32", "23.94.93.104/32", "23.94.93.106/32", "23.94.96.203/32", "23.95.242.213/32", "23.95.9.122/32", "23.95.9.156/32", "24.243.101.134/32", "2.59.40.240/32", "27.109.24.214/32", "31.184.255.100/32", "31.202.132.159/32", "31.202.132.95/32", "37.18.30.179/32", "37.18.30.55/32", "37.18.30.99/32", "37.44.212.120/32", "37.44.212.173/32", "37.44.212.182/32", "37.44.212.195/32", "37.44.212.196/32", "37.44.212.203/32", "37.44.212.86/32", "37.44.215.106/32", "37.44.215.107/32", "37.44.215.156/32", "37.44.215.169/32", "39.61.34.254/32", "41.202.77.180/32", "41.32.82.216/32", "43.229.62.186/32", "45.12.215.57/32", "45.15.253.25/32", "45.15.253.27/32", "45.15.253.61/32", "45.67.228.192/32", "45.67.228.231/32", "45.73.17.164/32", "45.8.229.101/32", "45.8.229.113/32", "46.30.41.155/32", "51.254.160.193/32", "51.254.69.233/32", "51.75.58.173/32", "51.83.52.180/32", "5.188.168.61/32", "5.188.41.101/32", "5.189.224.172/32", "5.196.192.222/32", "5.196.195.6/32", "5.196.247.4/32", "5.230.147.179/32", "5.253.63.106/32", "5.253.63.112/32", "5.253.63.119/32", "5.67.205.99/32", "58.171.215.214/32", "58.65.211.99/32", "5.9.167.178/32", "60.32.214.242/32", "60.48.253.12/32", "65.49.60.163/32", "66.70.218.38/32", "66.70.218.60/32", "69.251.12.43/32", "69.43.168.200/32", "69.43.168.215/32", "69.45.19.251/32", "71.244.60.231/32", "76.86.20.103/32", "77.220.64.57/32", "77.244.219.49/32", "78.100.187.118/32", "78.155.206.28/32", "78.155.206.85/32", "78.155.207.191/32", "78.186.5.109/32", "78.47.56.190/32", "79.137.119.212/32", "80.227.184.182/32", "80.82.115.164/32", "80.87.193.47/32", "80.87.199.224/32", "81.177.136.36/32", "81.177.140.60/32", "82.118.21.133/32", "82.118.21.139/32", "82.118.21.91/32", "82.118.22.87/32", "82.202.194.179/32", "82.202.194.182/32", "82.202.221.160/32", "82.211.30.202/32", "84.200.208.98/32", "85.104.59.244/32", "85.119.150.159/32", "85.143.218.104/32", "85.143.219.53/32", "85.143.223.89/32", "85.204.116.131/32", "85.204.116.132/32", "85.204.116.138/32", "85.204.116.151/32", "85.204.116.41/32", "85.204.116.45/32", "85.204.116.94/32", "85.204.116.95/32", "85.25.210.172/32", "85.25.33.71/32", "86.99.35.122/32", "87.106.145.51/32", "88.215.2.29/32", "88.97.26.73/32", "89.105.203.180/32", "89.186.26.179/32", "89.32.41.122/32", "89.32.41.126/32", "89.32.41.127/32", "89.32.41.164/32", "91.235.129.119/32", "91.235.129.166/32", "91.235.129.167/32", "91.235.129.212/32", "91.240.84.110/32", "91.240.84.166/32", "91.240.85.141/32", "91.240.85.19/32", "92.119.114.186/32", "92.38.171.12/32", "92.38.171.20/32", "93.189.42.220/32", "93.189.44.235/32", "93.189.47.93/32", "93.95.97.209/32", "93.95.97.68/32", "94.103.94.103/32", "94.140.125.143/32", "94.156.189.197/32", "95.181.179.96/32", "95.181.198.173/32", "95.46.8.206/32", "1.10.16.0/20", "1.19.0.0/16", "1.32.128.0/18", "2.56.255.0/24", "2.58.92.0/22", "2.59.151.0/24", "2.59.248.0/22", "2.59.252.0/22", "5.8.37.0/24", "5.101.221.0/24", "5.134.128.0/19", "5.188.10.0/23", "5.252.132.0/22", "5.253.56.0/22", "23.226.48.0/20", "24.233.0.0/19", "27.126.160.0/20", "27.146.0.0/16", "31.11.43.0/24", "31.222.200.0/21", "36.0.8.0/21", "36.37.48.0/20", "36.93.0.0/16", "36.116.0.0/16", "36.119.0.0/16", "37.44.228.0/22", "37.139.128.0/22", "37.148.216.0/21", "37.221.120.0/22", "37.246.0.0/16", "42.0.32.0/19", "42.1.128.0/17", "42.96.0.0/18", "42.128.0.0/12", "42.160.0.0/12", "42.194.12.0/22", "42.194.128.0/17", "42.208.0.0/12", "43.229.52.0/22", "43.236.0.0/16", "43.250.116.0/22", "43.252.80.0/22", "45.4.128.0/22", "45.4.136.0/22", "45.6.48.0/22", "45.8.72.0/22", "45.9.156.0/22", "45.9.208.0/22", "45.13.36.0/24", "45.13.39.0/24", "45.14.164.0/22", "45.41.0.0/18", "45.41.192.0/18", "45.43.128.0/18", "45.59.128.0/18", "45.65.188.0/22", "45.67.140.0/22", "45.67.144.0/22", "45.81.36.0/22", "45.117.208.0/22", "45.121.204.0/22", "45.146.224.0/19", "45.150.224.0/19", "45.153.224.0/19", "46.3.0.0/16", "46.232.192.0/21", "46.243.142.0/24", "49.8.0.0/14", "49.238.64.0/18", "58.14.0.0/15", "60.233.0.0/16", "61.11.224.0/19", "61.45.251.0/24", "66.231.64.0/20", "67.213.112.0/20", "67.219.208.0/20", "74.114.148.0/22", "79.110.17.0/24", "79.110.18.0/24", "79.110.19.0/24", "79.110.25.0/24", "79.110.48.0/22", "79.110.60.0/22", "79.173.104.0/21", "80.76.48.0/22", "81.31.192.0/22", "81.161.236.0/22", "82.115.208.0/22", "83.143.112.0/22", "83.171.204.0/22", "83.175.0.0/18", "83.219.96.0/22", "84.21.172.0/22", "84.238.160.0/22", "85.31.44.0/22", "85.121.39.0/24", "85.208.136.0/22", "85.209.132.0/22", "86.55.40.0/23", "86.55.42.0/23", "88.218.76.0/22", "91.197.196.0/22", "91.200.12.0/22", "91.200.248.0/22", "91.209.12.0/24", "91.212.104.0/24", "91.220.163.0/24", "91.230.252.0/23", "91.234.36.0/24", "91.236.74.0/23", "91.240.165.0/24", "91.247.76.0/22", "92.119.124.0/22", "92.249.28.0/22", "92.249.48.0/22", "93.179.89.0/24", "93.179.90.0/24", "93.179.91.0/24", "94.103.124.0/22", "94.154.160.0/22", "94.154.168.0/22", "94.154.172.0/22", "94.154.176.0/22", "94.154.180.0/22", "95.141.27.0/24", "95.214.24.0/22", "101.42.0.0/16", "101.134.0.0/15", "101.192.0.0/14", "101.202.0.0/16", "101.203.128.0/19", "101.248.0.0/15", "101.252.0.0/15", "102.11.224.0/19", "102.13.240.0/20", "102.18.224.0/19", "102.28.224.0/19", "102.29.224.0/19", "102.196.96.0/19", "102.211.224.0/19", "102.212.224.0/19", "102.228.0.0/16", "102.232.0.0/16", "102.240.0.0/16", "103.2.44.0/22", "103.16.76.0/24", "103.23.8.0/22", "103.32.0.0/16", "103.32.132.0/22", "103.34.0.0/16", "103.35.160.0/22", "103.36.64.0/22", "103.57.248.0/22", "103.69.212.0/22", "103.141.96.0/19", "103.144.0.0/16", "103.148.224.0/19", "103.155.224.0/19", "103.158.224.0/19", "103.167.224.0/19", "103.171.224.0/19", "103.174.224.0/19", "103.180.224.0/19", "103.189.224.0/19", "103.197.8.0/22", "103.205.84.0/22", "103.207.160.0/22", "103.215.80.0/22", "103.228.60.0/22", "103.229.36.0/22", "103.229.40.0/22", "103.230.144.0/22", "103.232.136.0/22", "103.232.172.0/22", "103.236.32.0/22", "103.239.28.0/22", "103.243.8.0/22", "104.153.244.0/22", "104.166.96.0/19", "104.207.64.0/19", "104.222.160.0/19", "104.233.0.0/18", "104.239.0.0/17", "104.243.192.0/20", "104.247.96.0/19", "104.250.192.0/19", "104.250.224.0/19", "106.95.0.0/16", "107.182.112.0/20", "107.182.240.0/20", "107.190.160.0/20", "109.206.236.0/22", "109.206.240.0/22", "110.41.0.0/16", "116.119.0.0/17", "116.144.0.0/15", "116.146.0.0/15", "117.58.0.0/17", "117.120.64.0/18", "119.42.52.0/22", "119.58.0.0/16", "119.232.0.0/16", "120.48.0.0/15", "121.46.124.0/22", "121.100.128.0/18", "122.129.0.0/18", "122.185.0.0/16", "123.136.80.0/20", "123.249.0.0/16", "124.20.0.0/16", "124.68.0.0/15", "124.157.0.0/18", "124.242.0.0/16", "125.31.192.0/18", "125.58.0.0/18", "125.169.0.0/16", "128.13.0.0/16", "128.24.0.0/16", "128.85.0.0/16", "128.188.0.0/16", "130.21.0.0/16", "130.148.0.0/16", "130.196.0.0/16", "130.222.0.0/16", "131.72.60.0/22", "131.72.208.0/22", "131.108.16.0/22", "131.108.232.0/22", "131.143.0.0/16", "131.200.0.0/16", "132.255.132.0/22", "134.18.0.0/16", "134.22.0.0/16", "134.23.0.0/16", "134.33.0.0/16", "134.62.0.0/15", "134.127.0.0/16", "134.172.0.0/16", "136.230.0.0/16", "137.19.0.0/16", "137.31.0.0/16", "137.33.0.0/16", "137.55.0.0/16", "137.72.0.0/16", "137.76.0.0/16", "137.105.0.0/16", "137.114.0.0/16", "137.171.0.0/16", "137.218.0.0/16", "138.31.0.0/16", "138.36.92.0/22", "138.36.136.0/22", "138.52.0.0/16", "138.59.4.0/22", "138.59.204.0/22", "138.94.120.0/22", "138.94.144.0/22", "138.94.216.0/22", "138.97.156.0/22", "138.122.192.0/22", "138.125.0.0/16", "138.185.116.0/22", "138.186.208.0/22", "138.216.0.0/16", "138.219.172.0/22", "138.228.0.0/16", "138.249.0.0/16", "139.28.172.0/22", "139.81.0.0/16", "139.188.0.0/16", "140.167.0.0/16", "141.98.0.0/22", "141.98.4.0/22", "141.136.22.0/24", "141.136.27.0/24", "141.178.0.0/16", "141.253.0.0/16", "142.102.0.0/16", "143.0.236.0/22", "143.49.0.0/16", "143.135.0.0/16", "145.231.0.0/16", "146.3.0.0/16", "146.183.0.0/16", "146.202.0.0/16", "146.252.0.0/16", "147.7.0.0/16", "147.16.0.0/14", "147.78.100.0/22", "147.119.0.0/16", "148.148.0.0/16", "148.154.0.0/16", "148.178.0.0/16", "148.185.0.0/16", "148.248.0.0/16", "149.118.0.0/16", "149.143.64.0/18", "150.10.0.0/16", "150.22.128.0/17", "150.25.0.0/16", "150.40.0.0/16", "150.107.106.0/23", "150.121.0.0/16", "150.126.0.0/16", "150.129.136.0/22", "150.129.212.0/22", "150.129.228.0/22", "150.141.0.0/16", "150.242.100.0/22", "150.242.120.0/22", "150.242.144.0/22", "151.212.0.0/16", "152.109.0.0/16", "152.147.0.0/16", "153.14.0.0/16", "153.52.0.0/14", "153.93.0.0/16", "155.11.0.0/16", "155.40.0.0/16", "155.66.0.0/16", "155.71.0.0/16", "155.73.0.0/16", "155.108.0.0/16", "155.204.0.0/16", "155.249.0.0/16", "157.115.0.0/16", "157.162.0.0/16", "157.186.0.0/16", "157.195.0.0/16", "158.54.0.0/16", "158.90.0.0/17", "158.249.0.0/16", "159.80.0.0/16", "159.85.0.0/16", "159.151.0.0/16", "159.174.0.0/16", "159.219.0.0/16", "159.223.0.0/16", "159.229.0.0/16", "160.14.0.0/16", "160.21.0.0/16", "160.115.0.0/16", "160.117.0.0/16", "160.180.0.0/16", "160.181.0.0/16", "160.188.0.0/16", "160.200.0.0/16", "160.235.0.0/16", "160.240.0.0/16", "160.255.0.0/16", "161.0.0.0/19", "161.0.68.0/22", "161.1.0.0/16", "161.66.0.0/16", "161.189.0.0/16", "162.208.124.0/22", "162.212.188.0/22", "162.213.232.0/22", "162.222.128.0/21", "163.47.19.0/24", "163.50.0.0/16", "163.53.247.0/24", "163.59.0.0/16", "163.127.224.0/19", "163.128.224.0/19", "163.216.0.0/19", "163.250.0.0/16", "163.254.0.0/16", "164.6.0.0/16", "164.40.188.0/22", "164.79.0.0/16", "164.137.0.0/16", "165.52.0.0/14", "165.102.0.0/16", "165.205.0.0/16", "165.209.0.0/16", "166.117.0.0/16", "167.74.0.0/18", "167.97.0.0/16", "167.103.0.0/16", "167.158.0.0/16", "167.160.96.0/19", "167.162.0.0/16", "167.175.0.0/16", "167.224.0.0/19", "167.249.200.0/22", "168.0.212.0/22", "168.64.0.0/16", "168.90.96.0/22", "168.90.108.0/22", "168.129.0.0/16", "168.151.0.0/22", "168.151.4.0/23", "168.151.6.0/24", "168.181.52.0/22", "168.195.76.0/22", "168.196.236.0/22", "168.196.240.0/22", "168.205.72.0/22", "168.227.128.0/22", "168.227.140.0/22", "170.67.0.0/16", "170.83.232.0/22", "170.113.0.0/16", "170.114.0.0/16", "170.120.0.0/16", "170.179.0.0/16", "170.244.40.0/22", "170.244.240.0/22", "170.245.40.0/22", "170.247.220.0/22", "171.22.16.0/22", "171.22.28.0/22", "171.25.0.0/17", "171.25.212.0/22", "171.26.0.0/16", "172.98.0.0/18", "174.136.192.0/18", "175.103.64.0/18", "176.56.192.0/19", "176.97.116.0/22", "176.125.252.0/22", "177.36.16.0/20", "177.74.160.0/20", "177.91.0.0/22", "177.234.136.0/21", "178.16.80.0/20", "178.215.224.0/22", "178.215.236.0/22", "179.42.64.0/19", "179.63.0.0/17", "180.178.192.0/18", "180.236.0.0/14", "181.177.64.0/18", "185.0.96.0/19", "185.17.96.0/22", "185.30.168.0/22", "185.35.136.0/22", "185.50.250.0/24", "185.50.251.0/24", "185.64.20.0/22", "185.68.156.0/22", "185.72.68.0/22", "185.84.183.0/24", "185.93.187.0/24", "185.103.72.0/22", "185.106.94.0/24", "185.129.208.0/22", "185.135.140.0/22", "185.137.219.0/24", "185.140.108.0/22", "185.147.140.0/22", "185.156.88.0/21", "185.156.92.0/22", "185.159.68.0/22", "185.165.24.0/22", "185.166.68.0/22", "185.167.116.0/22", "185.171.148.0/22", "185.175.140.0/22", "185.178.164.0/22", "185.184.192.0/22", "185.185.48.0/22", "185.187.236.0/22", "185.194.100.0/22", "185.196.96.0/22", "185.199.240.0/22", "185.202.88.0/22", "185.204.100.0/22", "185.204.236.0/22", "185.205.180.0/22", "185.208.128.0/22", "185.208.152.0/22", "185.209.240.0/22", "185.212.56.0/22", "185.212.244.0/22", "185.213.176.0/22", "185.215.132.0/22", "185.221.236.0/22", "185.222.211.0/24", "185.223.132.0/22", "185.223.244.0/22", "185.227.200.0/22", "185.234.100.0/22", "185.237.60.0/22", "185.239.24.0/22", "185.239.32.0/22", "185.239.104.0/22", "185.241.192.0/22", "185.242.120.0/22", "185.245.112.0/22", "185.246.220.0/22", "185.247.172.0/22", "185.252.160.0/22", "185.252.176.0/22", "186.65.112.0/20", "186.179.0.0/18", "188.172.160.0/19", "188.247.135.0/24", "188.247.230.0/24", "190.123.208.0/20", "190.185.108.0/22", "191.101.167.0/24", "192.5.103.0/24", "192.12.131.0/24", "192.22.0.0/16", "192.26.25.0/24", "192.31.212.0/23", "192.40.29.0/24", "192.43.154.0/23", "192.43.160.0/24", "192.43.175.0/24", "192.43.176.0/21", "192.43.184.0/24", "192.46.192.0/18", "192.54.110.0/24", "192.67.16.0/24", "192.88.74.0/24", "192.100.142.0/24", "192.101.44.0/24", "192.101.181.0/24", "192.101.200.0/21", "192.101.240.0/21", "192.101.248.0/23", "192.133.3.0/24", "192.135.255.0/24", "192.145.28.0/22", "192.145.52.0/22", "192.152.194.0/24", "192.154.11.0/24", "192.158.51.0/24", "192.160.44.0/24", "192.190.49.0/24", "192.190.97.0/24", "192.195.150.0/24", "192.197.87.0/24", "192.203.252.0/24", "192.206.114.0/24", "192.219.120.0/21", "192.219.128.0/18", "192.219.192.0/20", "192.219.208.0/21", "192.226.16.0/20", "192.229.32.0/19", "192.231.66.0/24", "192.234.189.0/24", "192.245.101.0/24", "192.251.231.0/24", "193.8.184.0/22", "193.9.158.0/24", "193.25.48.0/20", "193.25.216.0/22", "193.32.208.0/22", "193.35.16.0/22", "193.37.40.0/22", "193.37.44.0/22", "193.42.32.0/22", "193.46.172.0/22", "193.47.60.0/22", "193.58.120.0/22", "193.84.132.0/22", "193.109.108.0/22", "193.139.0.0/16", "193.148.48.0/22", "193.148.56.0/22", "193.148.92.0/22", "193.149.28.0/22", "193.168.196.0/22", "193.168.216.0/22", "193.187.96.0/22", "193.222.96.0/22", "193.243.0.0/17", "194.1.152.0/24", "194.11.196.0/22", "194.15.32.0/22", "194.15.44.0/22", "194.48.248.0/22", "194.55.184.0/22", "194.55.224.0/22", "194.59.28.0/22", "194.113.36.0/22", "194.169.172.0/22", "194.180.36.0/22", "194.180.48.0/22", "195.182.57.0/24", "195.191.102.0/23", "195.210.96.0/19", "196.1.109.0/24", "196.42.128.0/17", "196.63.0.0/16", "196.196.0.0/16", "196.199.0.0/16", "197.154.0.0/16", "197.159.80.0/21", "198.13.0.0/20", "198.20.16.0/20", "198.45.32.0/20", "198.45.64.0/19", "198.56.64.0/18", "198.57.64.0/20", "198.62.70.0/24", "198.62.76.0/24", "198.96.224.0/20", "198.99.117.0/24", "198.102.222.0/24", "198.148.212.0/24", "198.151.16.0/20", "198.151.64.0/18", "198.151.152.0/22", "198.160.205.0/24", "198.169.201.0/24", "198.177.175.0/24", "198.177.176.0/22", "198.177.180.0/24", "198.177.214.0/24", "198.178.64.0/19", "198.179.22.0/24", "198.181.64.0/19", "198.181.96.0/20", "198.183.32.0/19", "198.184.193.0/24", "198.184.208.0/24", "198.186.25.0/24", "198.186.208.0/24", "198.187.64.0/18", "198.187.192.0/24", "198.190.173.0/24", "198.199.212.0/24", "198.202.237.0/24", "198.204.0.0/21", "198.206.140.0/24", "198.212.132.0/24", "199.5.152.0/23", "199.5.229.0/24", "199.26.137.0/24", "199.26.207.0/24", "199.26.251.0/24", "199.33.222.0/24", "199.34.128.0/18", "199.46.32.0/19", "199.60.102.0/24", "199.71.56.0/21", "199.71.192.0/20", "199.84.55.0/24", "199.84.56.0/22", "199.84.60.0/24", "199.84.64.0/19", "199.88.32.0/20", "199.88.48.0/22", "199.89.16.0/20", "199.89.198.0/24", "199.120.163.0/24", "199.165.32.0/19", "199.166.200.0/22", "199.184.82.0/24", "199.185.192.0/20", "199.196.192.0/19", "199.198.160.0/20", "199.198.176.0/21", "199.198.184.0/23", "199.198.188.0/22", "199.200.64.0/19", "199.212.96.0/20", "199.223.0.0/20", "199.230.64.0/19", "199.230.96.0/21", "199.233.85.0/24", "199.233.96.0/24", "199.241.64.0/19", "199.244.56.0/21", "199.245.138.0/24", "199.246.137.0/24", "199.246.213.0/24", "199.246.215.0/24", "199.248.64.0/18", "199.249.64.0/19", "199.253.32.0/20", "199.253.48.0/21", "199.253.224.0/20", "199.254.32.0/20", "200.0.60.0/23", "200.22.0.0/16", "200.71.124.0/22", "200.189.44.0/22", "200.234.128.0/18", "201.148.168.0/22", "201.169.0.0/16", "202.0.192.0/18", "202.20.32.0/19", "202.21.64.0/19", "202.27.96.0/23", "202.27.98.0/24", "202.27.99.0/24", "202.27.100.0/22", "202.27.120.0/22", "202.27.161.0/24", "202.27.162.0/23", "202.27.164.0/22", "202.27.168.0/24", "202.39.112.0/20", "202.40.32.0/19", "202.40.64.0/18", "202.68.0.0/18", "202.86.0.0/22", "202.148.32.0/20", "202.148.176.0/20", "202.183.0.0/19", "202.189.80.0/20", "203.2.200.0/22", "203.9.0.0/19", "203.31.88.0/23", "203.34.70.0/23", "203.34.252.0/23", "203.86.252.0/22", "203.169.0.0/22", "203.191.64.0/18", "203.195.0.0/18", "204.19.38.0/23", "204.44.32.0/20", "204.44.224.0/20", "204.52.96.0/19", "204.52.255.0/24", "204.57.16.0/20", "204.75.147.0/24", "204.75.228.0/24", "204.80.198.0/24", "204.86.16.0/20", "204.87.199.0/24", "204.89.224.0/24", "204.106.128.0/18", "204.106.192.0/19", "204.107.208.0/24", "204.126.244.0/23", "204.128.151.0/24", "204.128.180.0/24", "204.130.16.0/20", "204.130.167.0/24", "204.147.64.0/21", "204.194.64.0/21", "204.225.16.0/20", "204.225.159.0/24", "204.225.210.0/24", "204.232.0.0/18", "204.238.137.0/24", "204.238.170.0/24", "204.238.183.0/24", "205.137.0.0/20", "205.142.104.0/22", "205.144.0.0/20", "205.144.176.0/20", "205.148.128.0/18", "205.148.192.0/18", "205.151.128.0/19", "205.159.45.0/24", "205.159.174.0/24", "205.159.180.0/24", "205.166.77.0/24", "205.166.84.0/24", "205.166.130.0/24", "205.166.168.0/24", "205.166.211.0/24", "205.172.176.0/22", "205.172.244.0/22", "205.175.160.0/19", "205.189.71.0/24", "205.189.72.0/23", "205.203.0.0/19", "205.203.224.0/19", "205.207.134.0/24", "205.210.107.0/24", "205.210.139.0/24", "205.210.171.0/24", "205.210.172.0/22", "205.214.96.0/19", "205.214.128.0/19", "205.233.224.0/20", "205.236.185.0/24", "205.236.189.0/24", "205.237.88.0/21", "206.41.160.0/19", "206.51.29.0/24", "206.124.104.0/21", "206.130.188.0/24", "206.143.128.0/17", "206.195.224.0/19", "206.197.28.0/24", "206.197.29.0/24", "206.197.77.0/24", "206.197.165.0/24", "206.209.80.0/20", "206.224.160.0/19", "206.226.0.0/19", "206.226.32.0/19", "206.227.64.0/18", "207.22.192.0/18", "207.32.128.0/19", "207.45.224.0/20", "207.110.64.0/18", "207.110.96.0/19", "207.110.128.0/18", "207.183.192.0/19", "207.226.192.0/20", "208.93.4.0/22", "209.51.32.0/20", "209.66.128.0/19", "209.95.192.0/19", "209.99.128.0/18", "209.145.0.0/19", "209.182.64.0/19", "209.229.0.0/16", "209.242.192.0/19", "212.87.204.0/22", "212.87.220.0/22", "212.115.40.0/22", "212.146.180.0/24", "216.83.208.0/20", "216.137.176.0/20", "217.197.168.0/22", "220.154.0.0/16", "221.132.192.0/18", "223.0.0.0/15", "223.169.0.0/16", "223.173.0.0/16", "223.254.0.0/16", "185.176.27.0/24", "81.22.45.0/24", "185.175.93.0/24", "89.248.174.0/24", "92.53.65.0/24", "198.108.67.0/24", "194.28.112.0/24", "120.52.152.0/24", "146.88.240.0/24", "125.64.94.0/24", "122.228.19.0/24", "217.61.20.0/24", "185.209.0.0/24", "185.216.140.0/24", "71.6.233.0/24", "37.49.227.0/24", "185.176.26.0/24", "89.248.160.0/24", "104.143.83.0/24", "80.82.64.0/24"};

typedef struct
{
    uint32_t srcip_prefix; // can be none
    uint32_t srcmask;  
    uint32_t dstip_prefix; // can be none
    uint32_t dstmask; 

    uint16_t srcport; // can be none
    uint16_t dstport; // can be none

    bool established; // can be none

    uint8_t valid_flag; // & of 0x1, 0x2, 0x4, 0x8, 0x10;

    bool drop; // only false or true
}acl_t;

acl_t acls[ACL_RULE_NUM * 2];
// = 
// {
//     {0x00000000, 0xFFFFFFFF, 0x0, 0xFFFFFFFF, 0x0, 0x0, false, 0x1, false}, 
//     {0x0, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x0, 0x0, false, 0x2, false}, 
//     {0x0, 0xFFFFFFFF, 0x0, 0xFFFFFFFF, 1338, 0x0, false, 0x4, false}, 
//     {0x0, 0xFFFFFFFF, 0x0, 0xFFFFFFFF, 0x0, 1338, false, 0x8, false}, 
//     {0x0, 0xFFFFFFFF, 0x0, 0xFFFFFFFF, 0x0, 0x0, true, 0x10, false}
// };

void fill_rules()
{
	int i = 0;
	for(; i < ACL_RULE_NUM; i++)
	{
		uint32_t a, b, c, d, len, ip_val;
		char * rule = FW_RULES[i];
		sscanf(rule, "%u.%u.%u.%u/%u", &a, &b, &c, &d, &len);
		ip_val = (a << 24) | (b << 16) | (c << 8) | d;
		acls[2 * i].srcip_prefix = ip_val;
		acls[2 * i].srcmask = (0xFFFFFFFF << (32 - len));
		acls[2 * i].dstip_prefix = 0x0;
		acls[2 * i].dstmask = 0xFFFFFFFF;
		acls[2 * i].srcport = 0x0;
		acls[2 * i].dstport = 0x0;
		acls[2 * i].established = false;
		acls[2 * i].valid_flag = 0x1;
		acls[2 * i].drop = false;

		acls[2 * i + 1].srcip_prefix = 0x0;
		acls[2 * i + 1].srcmask = 0xFFFFFFFF;
		acls[2 * i + 1].dstip_prefix = ip_val;
		acls[2 * i + 1].dstmask = (0xFFFFFFFF << (32 - len));
		acls[2 * i + 1].srcport = 0x0;
		acls[2 * i + 1].dstport = 0x0;
		acls[2 * i + 1].established = true;
		acls[2 * i + 1].valid_flag = (0x2 | 0x10);
		acls[2 * i + 1].drop = false;
	}
}


#define CONTAINS(ip, prefix, mask, flag) ((!flag) || (ip & mask) == prefix)

static inline bool matches(five_tuple_t flow, acl_t acl)
{
    if(CONTAINS(flow.srcip, acl.srcip_prefix, acl.srcmask, acl.valid_flag & 0x1) \
        && CONTAINS(flow.dstip, acl.dstip_prefix, acl.dstmask, (acl.valid_flag >> 1) & 0x1) \
        && ((!((acl.valid_flag >> 2) & 0x1)) || flow.srcport ==  acl.srcport) \
        && ((!((acl.valid_flag >> 3) & 0x1)) || flow.dstport ==  acl.dstport))
        {
            if((acl.valid_flag >> 4) & 0x1)
            {
                five_tuple_t rev_flow = REVERSE(flow);
                return (dleft_lookup(&ht_meta, flow) != NULL || dleft_lookup(&ht_meta, rev_flow) != NULL) == acl.established;
            }
            else
            {
                return true;
            }
        }
        else
        {
            return false;
        }
}
#define NTOHS(x) (((x & 0xF) << 4) | ((x >> 4) & 0xF))

void recover_hashmap(){
    FILE *file = fopen("/users/yangzhou/NFShield/data/acl_fw_hashmap_dleft.raw", "r");
    if (file == NULL){
        printf("recovery file open error");
        exit(0);
    }
    uint32_t srcip, dstip;
    uint16_t srcport, dstport;
    uint8_t proto;
    uint8_t val;
    uint32_t cnt = 0;
    while(fread(&srcip, sizeof(uint32_t), 1, file) != 0){
        fread(&dstip, sizeof(uint32_t), 1, file);
        fread(&srcport, sizeof(uint16_t), 1, file);
        fread(&dstport, sizeof(uint16_t), 1, file);
        fread(&proto, sizeof(uint8_t), 1, file);
        fread(&val, sizeof(uint8_t), 1, file);
        five_tuple_t flow = { .srcip = srcip, .dstip = dstip, .srcport = srcport, .dstport = dstport, .proto = proto };
        // if(cnt <= 8){
        //     printf("recover: %x %x %hu %hu %hu\n", flow.srcip, flow.dstip, flow.srcport, flow.dstport, flow.proto);
        // }
        cnt++;
        dleft_update(&ht_meta_cache, flow, val == 1);
    }
    printf("recover done\n");
}

int main(){
    if(-1 == dleft_init("acl-fw", HT_SIZE, &ht_meta))
    {
        printf("bootmemory allocation error\n");
        return 0;
    }
    if(-1 == dleft_init("acl-fw-meta", HT_SIZE, &ht_meta_cache))
    {
        printf("bootmemory allocation error\n");
        return 0;
    }
    fill_rules();

    srand((unsigned)time(NULL));

    load_pkt("/users/yangzhou/NFShield/data/ictf2010_100kflow.dat");
    
    uint32_t pkt_cnt = 0;
    uint32_t pkt_count_match = 0;
    
// #define FW_DUMP
#ifndef FW_DUMP
    recover_hashmap();
#endif

    while(1){
        pkt_t *raw_pkt = next_pkt();
        uint8_t *pkt_ptr = raw_pkt->content;
        uint16_t pkt_len = raw_pkt->len;
        swap_mac_addr(pkt_ptr);
        
        five_tuple_t flow;
        get_five_tuple(pkt_ptr, &flow);

// #define DEBUG
#ifdef DEBUG
        uint64_t src_mac = get_src_mac(pkt_ptr);
        uint64_t dst_mac = get_dst_mac(pkt_ptr);
        if(pkt_cnt <= 8){
            printf("%lx %lx %x %x %hu %hu %hu\n", src_mac, dst_mac, flow.srcip, flow.dstip, flow.srcport, flow.dstport, flow.proto);
        }
#endif

		bool flag = false;
		bool dropped = false;
		bool *res_ptr = dleft_lookup(&ht_meta_cache, flow);
		if(res_ptr != NULL)
		{
            // printf("hit!\n");
			dropped = *res_ptr;
		}
		else
		{
	        for(int i = 0; i < ACL_RULE_NUM_USED; i++)
	        {
	            if(matches(flow, acls[i]))
	            {
	                if(!acls[i].drop) // record the dropped packet info
	                {
	                    dleft_update(&ht_meta, flow, true);
	                }
	                flag = true;
	                dropped = false;
	                pkt_count_match ++;
	                break;
	            }
	        }
		    if(!flag)
		    {
		        dropped = true;
		    }
			dleft_update(&ht_meta_cache, flow, dropped);
		}

#ifdef FW_DUMP    
        if(pkt_cnt % PRINT_INTERVAL == 0) {
            printf("acl-fw %u\n", pkt_cnt);
        }
        if(pkt_cnt == 1024 * 1024) {
            dleft_dump(&ht_meta_cache, "/users/yangzhou/NFShield/data/acl_fw_hashmap_dleft.raw");
            break;
        }
#endif
        pkt_cnt ++;        
    }

    dleft_destroy(&ht_meta);
    dleft_destroy(&ht_meta_cache);
    return 0;
}