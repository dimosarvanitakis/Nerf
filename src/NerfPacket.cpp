#include "NerfPacket.h"
#include "Utilities.h"

const uint8_t NerfPacket::signature[SIGNATURE_LEN] = SIGNATURE;

// ======================================================================================================================================= 
// =============================================== Serialize/Deserialize =================================================================
// =======================================================================================================================================

bool CheckSignature(uint8_t* s1)
{
    uint8_t signature[SIGNATURE_LEN] = SIGNATURE;
    for(int i=0; i < SIGNATURE_LEN; i++ , s1++)
        if(*s1 != signature[i])
            return false;
    return true; 
}

void NerfPacket::Serialize(uint8_t* bufferToStore)
{
    uint8_t sendU8;
    uint32_t sendU32;

    for(int i = 0; i < SIGNATURE_LEN; i++)
        memcpy(bufferToStore + i, &signature[i], sizeof(uint8_t));

    memcpy(bufferToStore + SIGNATURE_LEN, &flags, sizeof(uint8_t));

    sendU32 = reverseBytes(lenght);
    memcpy(bufferToStore + SIGNATURE_LEN + 1, &sendU32, sizeof(uint32_t));

    for(int i = 0; i < lenght; i++)
    {
        sendU8 = reverseBytes(payload[i]);
        memcpy(bufferToStore + SIGNATURE_LEN + 5 + i, &sendU8, sizeof(uint8_t));
    }
}

NerfPacket NerfPacket::Deserialize(uint8_t* buffer)
{
    uint8_t signature[SIGNATURE_LEN];

    NerfPacket packet;  
 
    uint8_t sendU8;
    uint32_t sendU32;
    
    for(int i = 0; i < SIGNATURE_LEN; i++)
        memcpy(&signature[i] , buffer + i , sizeof(uint8_t));

    if(!CheckSignature(signature))
        return packet;

    memcpy(&packet.flags, buffer + SIGNATURE_LEN, sizeof(uint8_t));
    
    memcpy(&sendU32, buffer + SIGNATURE_LEN + 1, sizeof(uint32_t));
    packet.lenght = reverseBytes(sendU32);

    for(int i = 0; i < packet.lenght; i++)
    {
        memcpy(&sendU8, buffer + SIGNATURE_LEN + 5 + i, sizeof(uint8_t));
        packet.payload[i] = reverseBytes(sendU8);
    }

    return packet;
}

// ======================================================================================================================================= 
// ================================================== Useful Functions ===================================================================
// =======================================================================================================================================

NerfPacket NerfPacket::MakeSetupPacket(uint32_t udpPacketSize,
                                       uint16_t numberOfParallelStreams,
                                       uint8_t measureOneWay,
                                       double  printResultsInterval,
                                       uint8_t printResultInter)
{
    NerfPacket packet;

    packet.flags  = SETUP;
    packet.lenght = (sizeof(uint32_t) + (2 * sizeof(uint8_t)) + sizeof(uint16_t) + sizeof(double));

    memset(packet.payload , 0 , PAYLOAD_SIZE_IN_BYTES);

    memcpy(packet.payload,                    &udpPacketSize,             sizeof(uint32_t));
    memcpy(packet.payload + sizeof(uint32_t), &numberOfParallelStreams,   sizeof(uint16_t));
    memcpy(packet.payload + sizeof(uint32_t) + sizeof(uint16_t), &measureOneWay, sizeof(uint8_t));
    
    memcpy(packet.payload + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t),   
           &printResultsInterval,             
           sizeof(double));
    memcpy(packet.payload + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(double),   
           &printResultInter,             
           sizeof(uint8_t));

    return packet;
}

NerfPacket NerfPacket::MakePortNumberPacket(std::vector<uint16_t> ports)
{
    NerfPacket portsPacket;
    uint32_t   numberOfOpenPorts = ports.size(); 

    portsPacket.flags  = OPEN_PORTS;
    portsPacket.lenght = (numberOfOpenPorts * sizeof(int)) + sizeof(uint32_t);

    memset(portsPacket.payload , 0 , PAYLOAD_SIZE);

    memcpy(portsPacket.payload , &numberOfOpenPorts, sizeof(uint32_t));
    for(uint32_t port = 0; port < numberOfOpenPorts; port++)
        memcpy(portsPacket.payload + sizeof(uint32_t) + (port * sizeof(uint16_t)), &ports[port], sizeof(uint16_t));

    return portsPacket;
}

NerfPacket NerfPacket::MakeClosePacket()
{
    NerfPacket packet;
    
    packet.flags  = CLOSE;
    packet.lenght = 0;

    return packet;
}

NerfPacket NerfPacket::MakeLastSequenceNumberPacket(uint16_t port , uint64_t lastPacketSend)
{
    NerfPacket packet;
    
    packet.flags  = LAST_PACKET;
    packet.lenght = sizeof(uint16_t) + sizeof(uint64_t);

    memset(packet.payload, 0, PAYLOAD_SIZE);

    memcpy(packet.payload,                    &port,           sizeof(uint16_t));
    memcpy(packet.payload + sizeof(uint16_t), &lastPacketSend, sizeof(uint64_t));

    return packet;
}

NerfPacket NerfPacket::MakeStartPacket()
{
    NerfPacket packet;

    packet.flags  = START;
    packet.lenght = 0;

    return packet;
}

NerfPacket NerfPacket::MakeMeasurementsPacket(uint8_t oneWayDelayMes, 
                                              double averageThroughput, 
                                              double averageGoopput, 
                                              double packetloss, 
                                              double jitter, 
                                              double jitterDeviation)
{
    NerfPacket packet;

    packet.flags    = MEASUREMENT;
    packet.lenght   = (sizeof(uint8_t) + (5 * sizeof(double)));

    memset(packet.payload, 0, PAYLOAD_SIZE_IN_BYTES);
    
    memcpy(packet.payload, &oneWayDelayMes, sizeof(uint8_t));
    memcpy(packet.payload + sizeof(uint8_t),                        &averageThroughput, sizeof(double));
    memcpy(packet.payload + sizeof(uint8_t) + sizeof(double),       &averageGoopput,    sizeof(double));
    memcpy(packet.payload + sizeof(uint8_t) + (2 * sizeof(double)), &packetloss,        sizeof(double));
    memcpy(packet.payload + sizeof(uint8_t) + (3 * sizeof(double)), &jitter,            sizeof(double));
    memcpy(packet.payload + sizeof(uint8_t) + (4 * sizeof(double)), &jitterDeviation,   sizeof(double));

    return packet;
}

NerfPacket NerfPacket::MakeMeasurementsPacket(uint8_t oneWayDelayMes , double oneWayDelay)
{
    NerfPacket packet;

    packet.flags    = MEASUREMENT;
    packet.lenght   = (sizeof(uint8_t) + sizeof(double));

    memset(packet.payload, 0, PAYLOAD_SIZE_IN_BYTES);
    
    memcpy(packet.payload, &oneWayDelayMes, sizeof(uint8_t));
    memcpy(packet.payload + sizeof(uint8_t), &oneWayDelay, sizeof(double));

    return packet;
}
