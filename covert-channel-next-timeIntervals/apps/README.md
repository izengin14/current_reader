This repository contains 2 open source android apps written in Kotlin. Within the apps are transmitter and receiver functions whose purpose is to establish a covert channel communication.
Both the transmitter and reciever functions are called within the Android Apps in the MainActivity.kt file. Specifically they are called in the onCreate function.

Transmitter:

The transmitter is located in the contacts app within the pro.cpp file and is written in C++. This function performs timed memory access in a loop according to an encoded bitstring of 1s and 0s.
The timed memory accesses are performed by copying a very large array of floats to a new array. 
The encoded bitstring is created by getting a list of contacts from the Contact app in Kotlin and transforming it within the pro.cpp file

Receiver:

The receiver is located in the notes app wihtin the pro.cpp file and is also written in C++. This function performs a loop with a very similar array copy as the transmitter, but only every 300ms.
When it does this, it measures the time it takes to perform the copy and records that time. Next iteration of the loop, it checks if the time it takes to copy the loop is longer than expected and if so it records a memory spike.

Known Errors:

-Transmitter isn't synced with the receiver and doesn't send a signal that tells the reciever a message is being sent

-Receiver needs to be fine-tuned and synced with the transmitter, it can sense spikes in shared memory usage but it picks up on background noise

