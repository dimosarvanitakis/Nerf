#ifndef _UTILITIES_H_
#define _UTILITIES_H_

// ======================================================================================================================================= 
// =================================================  Shared Includes ====================================================================
// =======================================================================================================================================

#include <cstring>
#include <cstdint>
#include <thread>
#include <chrono>
#include <algorithm>
#include <time.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

// ======================================================================================================================================= 
// =================================================  DEFINES ============================================================================ 
// =======================================================================================================================================

#define DEFAULT_UDP_PACKET_SIZE            1024    // 1Kb
#define DEFAULT_NUMBER_OF_PARALLEL_STREAMS 0       // none  
#define DEFAULT_MEASURE_ONE_WAY            0       // false
#define DEFAULT_PRINT_IN_FILE              0       // false
#define DEFAULT_INTERVAL_TO_PRINT          0       // print only in the end
#define DEFAULT_BANDWIDTH                  1000000 // 1Mbits/sec

#define ONE_SECOND_TO_NANO                 1000000000

#define UDP_HEADER_SIZE                     8
#define TCP_HEADER_SIZE                     20
#define IPV4_HEADER_SIZE                    20
#define ETH_HEADER_SIZE                     26

#define HEADERS_FROM_THE_LAYERS            (UDP_HEADER_SIZE + IPV4_HEADER_SIZE + ETH_HEADER_SIZE)

// ======================================================================================================================================= 
// ==================================================  Time ============================================================================== 
// =======================================================================================================================================

using Time = struct timespec;

struct SystemClock
{
    static void GetSystemTime(Time* fill);

    static Time GetElapsedTime(Time* begin , Time* end);
    
    static double GetTimeInSeconds(Time* time);
    
    static uint64_t GetTimeInNanoSeconds(Time* time);

    static std::size_t Serialize(Time* time , uint8_t* buffer , std::size_t padding);

    static std::size_t Derialize(Time* time , uint8_t* buffer , std::size_t padding);
};

// ======================================================================================================================================= 
// ===========================================  Useful Functions  ======================================================================== 
// =======================================================================================================================================

template <typename T> static inline T reverseBytes(const T &input)
{
#if BYTE_ORDER == BIG_ENDIAN
    return input;
#elif BYTE_ORDER == LITTLE_ENDIAN
    T output = T(input);

    const std::size_t size = sizeof(input);

    uint8_t* p = reinterpret_cast<uint8_t*>(&output);
    
    for (size_t i = 0; i < size / 2; ++i)
        std::swap(p[i], p[size - i - 1]);
    
    return output;
#else
    # error "Wait what...you are not little endia or big endian so what kind of machine are you?" 
#endif
}

void PrintUsage();

#endif