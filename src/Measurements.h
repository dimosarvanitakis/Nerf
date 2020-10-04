#ifndef _MEASUREMENTS_H_
#define _MEASUREMENTS_H_

#include <cstdint>

struct Measurements
{
public:
    //Throughtput , Goodput
    uint64_t totalBytesReceived;
    uint64_t totalBytesReceivedWll;
    double   averageThroughtput;
    double   averageGoodput;
    double   timeUntilNow;
    
    //Jitter and the standard deviation
    double jitter;
    double variance;
    double totalJitter;
    double jitterDeviation;
    double mean;

    //Packet lost
    uint64_t packetLost;
    uint64_t totalPackets;
    uint64_t totalPacketsThatTheClientHaveSend;

    //One Way measurements
    double oneWayDelay;
public:

    Measurements();

    void Reset();

    // ======================================================================================================================================= 
    // =================================================  Throughput ========================================================================= 
    // ======================================================================================================================================= 

    double GetThroughtput();

    static void CombineThroughtputs(Measurements* mes1 , const Measurements* mes2);

    // ======================================================================================================================================= 
    // =================================================  Goodput ============================================================================ 
    // ======================================================================================================================================= 

    double GetGoodput();

    static void CombineGoodputs(Measurements* mes1 , const Measurements* mes2);

    // ======================================================================================================================================= 
    // =====================================================  Jitter ========================================================================= 
    // ======================================================================================================================================= 

    double GetJitter();

    double GetJitterStandardDeviation();
    
    double PushJitter(double nowJitter);
    
    static void CombineJitters(Measurements* mes1 , const Measurements* mes2);
    
    static void CombineJittersDeviations(Measurements* mes1 , const Measurements* mes2);

    // ======================================================================================================================================= 
    // ================================================== Packet Lost ======================================================================== 
    // ======================================================================================================================================= 
   
    double GetPacketLostPercentage();

    static void CombinePacketLost(Measurements* mes1 , const Measurements* mes2);
 
    // ======================================================================================================================================= 
    // ================================================== One Way Delay ====================================================================== 
    // ======================================================================================================================================= 
    
    double GetOneWayDelay();

    static void CombineOneWayDelay(Measurements* mes1 , const Measurements* mes2);
};

#endif