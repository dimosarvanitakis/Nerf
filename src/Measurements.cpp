#include "Measurements.h"

#include <math.h>

Measurements::Measurements() 
{
    Reset();
};

void Measurements::Reset()
{
    //Reset throughtput and goodput
    totalBytesReceived    = 0;
    totalBytesReceivedWll = 0;
    averageThroughtput    = 0.0f;
    averageGoodput        = 0.0f;
    timeUntilNow          = 0.0f;

    //Reset packet lost
    packetLost   = 0;
    totalPackets = 0;
    totalPacketsThatTheClientHaveSend = 0;
    
    //Reset jitter and the standard deviation
    jitter          = 0.0f;
    variance        = 0.0f;
    totalJitter     = 0.0f;
    jitterDeviation = 0.0f;
    mean            = 0.0f;

    //Reset one way delay
    oneWayDelay = 0.0f;
}

// ======================================================================================================================================= 
// =================================================  Throughput ========================================================================= 
// ======================================================================================================================================= 

double Measurements::GetThroughtput()
{
    //return the throughtput in Mbits / second
    return averageThroughtput;
}

void Measurements::CombineThroughtputs(Measurements* mes1 , const Measurements* mes2)
{
    //README
    //Epeidh esteila mail kai h aporia mou den lu8hke...an bgaleis apo ta sxolia auto perneis to "average" anamesa se 2 average throughtput
    //To idio isxuei parakatw gia to goodput.

    // mes1->averageThroughtput =  (mes1->averageThroughtput * (mes1->timeUntilNow / (mes1->timeUntilNow + mes2->timeUntilNow))) +
    //                             (mes2->averageThroughtput * (mes2->timeUntilNow / (mes1->timeUntilNow + mes2->timeUntilNow)));
    
    mes1->averageThroughtput = (mes1->averageThroughtput + mes2->averageThroughtput);
}

// ======================================================================================================================================= 
// =================================================  Goodput ============================================================================ 
// =======================================================================================================================================

double Measurements::GetGoodput()
{
    //return the goodput in Mbits / second
    return averageGoodput;
}

void Measurements::CombineGoodputs(Measurements* mes1 , const Measurements* mes2)
{
    // mes1->averageGoodput =  (mes1->averageGoodput * (mes1->timeUntilNow / (mes1->timeUntilNow + mes2->timeUntilNow))) +
    //                         (mes2->averageGoodput * (mes2->timeUntilNow / (mes1->timeUntilNow + mes2->timeUntilNow)));
    
    mes1->averageGoodput = (mes1->averageGoodput + mes2->averageGoodput);
}

// ======================================================================================================================================= 
// =====================================================  Jitter ========================================================================= 
// ======================================================================================================================================= 

double Measurements::GetJitter()
{
    //return the jitter in ms
    return jitter * 1000;
}

double Measurements::GetJitterStandardDeviation()
{
    return jitterDeviation;
}

double Measurements::PushJitter(double nowJitter)
{
    variance = ((totalPackets - 1) / (double)totalPackets) * 
               ( variance + (pow((nowJitter - mean),2.0) / (double)totalPackets));
    
    totalJitter += nowJitter;

    mean = totalJitter / totalPackets;

    jitterDeviation = sqrt(variance);
}

void Measurements::CombineJitters(Measurements* mes1 , const Measurements* mes2)
{
    //Compine 2 average jitters

    mes1->jitter =  (mes1->jitter * 16/32) + (mes2->jitter * 16/32);
}

void Measurements::CombineJittersDeviations(Measurements* mes1 , const Measurements* mes2)
{
    //Combine 2 standard deviations 

    double mean;
    double standardDeviation;

    mean  = ((mes1->totalPackets * mes1->mean) + (mes2->totalPackets * mes2->mean)) / (mes1->totalPackets + mes2->totalPackets);

    standardDeviation = ( ((mes1->totalPackets - 1) * (mes1->jitterDeviation * mes1->jitterDeviation)) + 
                          ((mes2->totalPackets - 1) * (mes2->jitterDeviation * mes2->jitterDeviation)) +
                          (mes1->totalPackets * pow((mes1->mean - mean) , 2)) +
                          (mes2->totalPackets * pow((mes2->mean - mean) , 2)) ) /
                          (mes1->totalPackets + mes2->totalPackets - 1);
    
    mes1->jitterDeviation = sqrt(standardDeviation);
}

// ======================================================================================================================================= 
// ================================================== Packet Lost ======================================================================== 
// ======================================================================================================================================= 

double Measurements::GetPacketLostPercentage()
{
    //return the packet lost percentage
    return 100.0 * (double) ( (double) packetLost / (double) totalPacketsThatTheClientHaveSend);
}

void Measurements::CombinePacketLost(Measurements* mes1 , const Measurements* mes2)
{
    //compine the packet lost and the total packets between 2 streams

    mes1->totalPackets += mes2->totalPackets;
    mes1->packetLost   += mes2->packetLost;
    mes1->totalPacketsThatTheClientHaveSend += mes2->totalPacketsThatTheClientHaveSend; 
}

// ======================================================================================================================================= 
// ================================================== One Way Delay ====================================================================== 
// =======================================================================================================================================

double Measurements::GetOneWayDelay()
{
    //return the one way delay in ms
    return oneWayDelay * 1000;
}

void Measurements::CombineOneWayDelay(Measurements* mes1 , const Measurements* mes2)
{
    //Compine 2 one way delays 

    mes1->oneWayDelay = ((mes1->oneWayDelay + mes2->oneWayDelay) / 2.0); 
}