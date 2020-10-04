#ifndef _NERF_PACKET_H_
#define _NERF_PACKET_H_

#include <cstdint>
#include <vector>

#define SIGNATURE_LEN           4
#define PAYLOAD_SIZE            100

#define SIGNATURE               {'n' , 'e' , 'r' , 'f'}

#define NERF_PACKET_SIZE        (SIGNATURE_LEN + PAYLOAD_SIZE + 5)
#define NERF_PACKET_IN_BYTES    (NERF_PACKET_SIZE * sizeof(uint8_t))
#define PAYLOAD_SIZE_IN_BYTES   (PAYLOAD_SIZE * sizeof(uint8_t))

#define SETUP       1
#define START       2
#define CLOSE       3
#define MEASUREMENT 4
#define ERROR       5
#define OPEN_PORTS  6
#define LAST_PACKET 7

struct NerfPacket
{
    static const uint8_t signature[SIGNATURE_LEN];
    uint8_t     flags;
    uint32_t    lenght;
    uint8_t     payload[PAYLOAD_SIZE];

    NerfPacket() {};

    // ======================================================================================================================================= 
    // =============================================== Serialize/Deserialize =================================================================
    // =======================================================================================================================================

    void Serialize(uint8_t* bufferToStore);

    static NerfPacket Deserialize(uint8_t* buffer);

    // ======================================================================================================================================= 
    // ================================================== Useful Functions ===================================================================
    // =======================================================================================================================================
    
    static NerfPacket MakeSetupPacket(uint32_t udpPacketSize,
                                      uint16_t numberOfParallelStreams, 
                                      uint8_t measureOneWay,
                                      double printResultsInterval,
                                      uint8_t printResultInter
                                     );
    
    static NerfPacket MakeClosePacket();

    static NerfPacket MakeLastSequenceNumberPacket(uint16_t port , uint64_t lastPacketSend);

    static NerfPacket MakePortNumberPacket(std::vector<uint16_t> ports);

    static NerfPacket MakeStartPacket();

    static NerfPacket MakeMeasurementsPacket(uint8_t oneWayDelayMes, 
                                             double averageThroughput, 
                                             double averageGoopput, 
                                             double packetloss, 
                                             double jitter, 
                                             double jitterDeviation);
    
    static NerfPacket MakeMeasurementsPacket(uint8_t oneWayDelayMes , double oneWayDelay);
};

#endif