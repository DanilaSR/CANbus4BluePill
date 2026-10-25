ip link set can0 down
ip link set can0 type can bitrate 500000
ip link set can0 up
 echo `ip link show`
# (S0=10k S1=20k S2=50k S3=100k S4=125k S5=250k S6=500k S7=800k S8=1M)

