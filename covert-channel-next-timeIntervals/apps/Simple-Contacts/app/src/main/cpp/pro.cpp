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
#include <iostream>
#include <chrono>
#include <thread>


// Stream Parameters
#define STREAM_ARRAY_SIZE 8000000   //size of array being copied
#define LOOPS 1     //number of times to copy the array
#define BYTES (2 * STREAM_ARRAY_SIZE * sizeof(float16_t))

// used for logging
#define APPNAME "ContactsAppCPPDemo"

static float16_t a[STREAM_ARRAY_SIZE], b[STREAM_ARRAY_SIZE];
uint64_t loopTimes[LOOPS];

extern "C" {
    void sleep_milliseconds(int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
}

//returns the value of the clock register which is increased every cycle
extern "C" {
    static inline uint64_t read_pmccntr() {
        uint64_t val;
        asm volatile("mrs %0, cntvct_el0"
                : "=r"(val)); // pmccntr_el0
        return val;
    }
}

//measures CPU performance
extern "C" {
    static inline uint64_t get_clock_frequency() {
        uint64_t val;
        asm volatile("mrs %0, cntfrq_el0"
                : "=r"(val));
        return val;
    }
}

//calculate overhead for time
extern "C" {
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
}

/*
 * //vector instruction copy function (copies values of one array to another to simulate memory accesses)
extern "C" {
    void STREAM_copy_simd() {
        uint32_t i;
    #pragma omp parallel for
        for (i = 0; i < STREAM_ARRAY_SIZE; i += 8) {
            float16x8_t v1 = vld1q_f16(&a[i]);
            vst1q_f16(&b[i], v1);
        }
    }
}
 */

extern "C" {
void STREAM_copy_simd() {
    uint32_t i;
    float16x8_t v[16];  // Array of 16 NEON registers

    // Initialize the NEON registers. Using multiple will avoid caching
    for (int j = 0; j < 16; j++) {
        v[j] = vdupq_n_f16(0.0f);
    }

    for (i = 0; i < STREAM_ARRAY_SIZE; i += 8) {
        // Switch between different NEON registers in each iteration
        int regIndex = i / 8 % 16;  // Corrected indexing

        float16x8_t currentReg = v[regIndex];
        currentReg = vld1q_f16(&a[i]);
        vst1q_f16(&b[i], currentReg);

        // Update the NEON register in the array
        v[regIndex] = currentReg;

        // Print the register number (1-16)
//        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Register updated: %d", regIndex + 1);
    }
}
}

//array copy function (copies values of one array to another to simulate memory accesses)
extern "C" {
    void STREAM_copy() {
        uint32_t i;
    #pragma omp parallel for
        for (i = 0; i < STREAM_ARRAY_SIZE; i++) {
            b[i] = a[i];
        }
    }
}

//encrypts a string into a string of bits that has been flipped
extern "C" {
    std::string encryptData(std::string text) {
        std::string binary = "";
        for (char c: text) {
            binary += std::bitset<8>(c).flip().to_string();    //turn every char into an 8 bit representation and then flip all bits
        }
        return binary;
    }
}

////main c++ function, called in MainActivity of app (windows edition)
extern "C" {
JNIEXPORT jint /* JNICALL  this code for whatever reason isn't needed but it complains if I don't have it */
   Java_com_simplemobiletools_contacts_pro_activities_MainActivity_00024Companion_transmitter(JNIEnv *env, jobject thiz, jobjectArray contacts) {

        int stringCount = env->GetArrayLength(contacts);    // get length of array
        std::string contactData[stringCount];   // make local array of same size

        for (int x = 0; x < stringCount; x++) { //copy all contact data from the jobjectArray into a c string array
            jstring string = (jstring) (env->GetObjectArrayElement(contacts, x));   // get the value from index x

            // awful looking copying method
            jboolean isCopy;
            const char *convertedValue = (env)->GetStringUTFChars(string, &isCopy);
            std::string str = convertedValue;
            contactData[x] = str;
            //current attempt is casting string to char array bc that's the format that java expects for logcat
            __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%s", str.c_str());   //print to logcat for proof
        }

        //encrypt data:
        std::string encryptedStr = "";
        for(int y = 0; y < stringCount; y++){
            encryptedStr += encryptData(contactData[y]);
            __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%s", encryptedStr.c_str());   //print to logcat for proof
        }

        uint64_t dtMin, start, end, hz;
        uint32_t i, currentLoop;
        double total_time = 0;

        // Get the smallest overhead for measuring time
        dtMin = calibrate();

        // perform array copy and record the time it takes to do so
        for(int j = 0; j < encryptedStr.length(); j++) {
            if(encryptedStr[j] == '0'){
                sleep_milliseconds(300);     //this time should be fine tuned to be similar to the time of the else statement
            } else {    //perform array copy LOOP times
                for (currentLoop = 0; currentLoop < LOOPS; currentLoop++) {
                    start = read_pmccntr();
                    STREAM_copy_simd();
                    end = read_pmccntr();

                    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Transmission sent");
                    loopTimes[currentLoop] = end - start - dtMin;
                    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Time for copy: %d", loopTimes[currentLoop]); //if error, use %f
                }
            }
        }


        double time, bw;
        hz = get_clock_frequency();
        for (i = 0; i < currentLoop; i++) {
            time = (double) loopTimes[i] / hz;
            bw = BYTES / time / (1024 * 1024 * 1024);
            total_time = total_time + time;
        }

        return 0;
      }
}

////main c++ function, called in MainActivity of app (mac edition)
//this section exists because there was a weird error happening on windows/mac where if JNICALL was included in the header it didnt work for windows machines
/*
extern "C"
JNIEXPORT jint JNICALL
Java_com_simplemobiletools_contacts_pro_activities_MainActivity_00024Companion_transmitter(
        JNIEnv *env, jobject thiz, jobjectArray contacts) {
    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "CPP FILE LOG TEST");

    uint64_t dtMin, start, end, hz;
    uint32_t i, currentLoop;
    double total_time = 0;

    //calculate overhead time
    dtMin = calibrate();

    // Warmup
    for (i = 0; i < 1000; i++) {
        STREAM_copy_simd();
    }

    // Run for a fixed number of iterations
    for (currentLoop = 0; currentLoop < LOOPS; currentLoop++) {
        start = read_pmccntr();

        // Introduce a delay before each memory access
        sleep_milliseconds(20); //should be tuned with the receiver
        STREAM_copy_simd();
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Transmission sent");
        end = read_pmccntr();

        loopTimes[currentLoop] = end - start - dtMin;
    }


    double time, bw;
    hz = get_clock_frequency();
    for (i = 0; i < currentLoop; i++) {
        time = (double) loopTimes[i] / hz;
        bw = BYTES / time / (1024 * 1024 * 1024);
        total_time = total_time + time;
    }

    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "CPP FILE LOG TEST");
    return 0;
}*/
