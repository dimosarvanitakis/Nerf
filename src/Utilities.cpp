#include <stdio.h>

#include "Utilities.h"

// ======================================================================================================================================= 
// ==================================================  Time ============================================================================== 
// =======================================================================================================================================

void SystemClock::GetSystemTime(Time* fill)
{
    if( clock_gettime(CLOCK_MONOTONIC , fill) < 0)
        fprintf(stderr,"[Error ~ Time] : unable to get system time!\n");
}

Time SystemClock::GetElapsedTime(Time* time1 , Time* time2)
{
    Time diff;
    diff.tv_sec  = 0;
    diff.tv_nsec = 0;
    
    if((time1->tv_sec == time2->tv_sec) &&
       (time1->tv_nsec == time2->tv_nsec))
    {
        return diff;
    }

    if(time1->tv_sec > time2->tv_sec)
    {
        diff.tv_sec = time1->tv_sec - time2->tv_sec;
        
        if(time1->tv_nsec < time2->tv_nsec)
        {
            diff.tv_sec  -= 1;
            diff.tv_nsec  = (time1->tv_nsec + 1000000000) - time2->tv_nsec;
        }else
            diff.tv_nsec = time1->tv_nsec - time2->tv_nsec;
    }
    else
    {
        diff.tv_sec = time2->tv_sec - time1->tv_sec;

        if(time2->tv_nsec < time1->tv_nsec)
        {
            diff.tv_sec  -= 1;
            diff.tv_nsec  = (time2->tv_nsec + 1000000000) - time1->tv_nsec;
        }else
            diff.tv_nsec = time2->tv_nsec - time1->tv_nsec;
    }

    return diff;
}

double SystemClock::GetTimeInSeconds(Time* time)
{
    return (double) (time->tv_sec + (time->tv_nsec / 1000000000.0));
}

uint64_t SystemClock::GetTimeInNanoSeconds(Time* time)
{
    auto duration =  std::chrono::seconds{time->tv_sec} + std::chrono::nanoseconds{time->tv_nsec};

    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

std::size_t SystemClock::Serialize(Time* time , uint8_t* buffer, std::size_t padding)
{
    uint32_t sendSeconds;
    uint32_t sendNSeconds;

    sendSeconds  = time->tv_sec;
    sendNSeconds = time->tv_nsec;

    sendSeconds  = reverseBytes(sendSeconds);
    sendNSeconds = reverseBytes(sendNSeconds);

    memcpy(buffer + padding,                    &sendSeconds,  sizeof(uint32_t));
    memcpy(buffer + padding + sizeof(uint32_t), &sendNSeconds, sizeof(uint32_t));

    return (padding + (2 * sizeof(uint32_t)));
}
    
std::size_t SystemClock::Derialize(Time* time, uint8_t* buffer , std::size_t padding)
{
    uint32_t sendSeconds;
    uint32_t sendNSeconds;

    memcpy(&sendSeconds,  buffer + padding,                    sizeof(uint32_t)); 
    memcpy(&sendNSeconds, buffer + padding + sizeof(uint32_t), sizeof(uint32_t)); 

    time->tv_sec  = reverseBytes(sendSeconds);
    time->tv_nsec = reverseBytes(sendNSeconds);

    return (padding + (2 * sizeof(uint32_t)));
}

// ======================================================================================================================================= 
// ===========================================  Useful Functions  ======================================================================== 
// =======================================================================================================================================

void PrintUsage()
{
    fprintf(stdout,
                "\n"
                "Usage:\n"
                "      nerf -c [client options] {the programm runs as client}\n"
                "      nerf -s [server options] {the programm runs as server}\n");
    fprintf(stdout,
                "\n"
                "General Options:\n"
                "                -a   In server mode, this argument specifies the IP address of the network interface\n"
                "                     that the program should bind.In client mode, this argument provides the IP address\n"
                "                     of the server to connect to.\n"
                "                -p   In server mode this argument indicates the listening port of the primary\n"
                "                     communication TCP channel of the server. In client mode, the argument\n"
                "                     specifies the server port to connect to.\n"
                "                -i   The interval in seconds to print information for the progress of the experiment.\n"
                "                -f   Specifies the file that the results will be stored.");
    fprintf(stdout,   
                "\n"
                "Client Options:\n"
                "                -l   UDP packet size in bytes.\n"
                "                -b   The bandwidth in bits per second of the data stream that the client should\n"
                "                     sent to the server.\n"
                "                -n   Number of parallel data streams that the client should create.\n"
                "                -t   Experiment duration in seconds.\n"
                "                -d   Measure the one way delay, instead of throughput, jitter and packet loss.\n"
                "                -w   Wait duration in seconds before starting the data transmission.");
    fprintf(stdout,   
                "\n"
                "Other Options:\n"
                "                -h   Prints this help message.\n"
                "\n");
}