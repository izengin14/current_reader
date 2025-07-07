#This bash is to run the ./build/tranmistter and track the power consumption in DRAM the same time; the power consumption tracking will start 1 second before the trasmitter sends out the message
#!/bin/bash

# Receiver
# (
#   cd ~/Desktop/covert-channel-next-timeIntervals || exit
#   ./build/receiver copy 0 32 1024 104857600 > ./src/experiments/receiver/bandwidth.csv
# ) &

# Wait 5 second before starting command2
#sleep 5

# Start reading current on DRAM
(
  cd ~/Desktop/currentReader || exit
  python3 toOutputTxt.py 6
) &

# Wait 1 second before starting command2
sleep 1

# Transmitter
(
  cd ~/Desktop/currentReader/covert-channel-next-timeIntervals || exit
  ./build/transmitter copy 0 0 1073741824 "e" 100 10   # ascii code for e is "01100101"
) &

wait

#Transmitter buffer size:
#1 GiB= 1073741824 bytes
#1/2 GiB= 536870912
#1/4 GiB= 268435456
#1/8 GiB= 134217728
#1/16 GiB= 67108864
#1/32 GiB= 33554432
#1/64 GiB= 16777216

#receiver:
#original: 104857600
#half: 52428800
#quater: 26214400