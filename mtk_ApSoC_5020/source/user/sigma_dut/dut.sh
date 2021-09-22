export WFA_WIRELESS_DUT_IP=192.168.250.78
export WFA_WIRELESS_INTERFACE=apclii0
ifconfig eth0 0.0.0.0
./dut/wfa_dut eth0 8000
