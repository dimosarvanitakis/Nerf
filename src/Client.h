#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "Utilities.h"
#include "NerfPacket.h"

#define DEFAULT_SERVER_PORT_TO_SEND    3742
#define DEFAULT_SERVER_IP_TO_SEND      "127.0.0.1"  

struct ClientStreamParams
{
    int socketId;
    uint16_t port;
    struct sockaddr_in serverToSendData;

    int64_t  totalBytesSend;

    uint64_t udpSeqNumber;

    uint32_t udpPacketSize;
    uint64_t bandwidth;
    uint64_t durationInSeconds;

    bool stop;
};

class Client
{ 
private: 
    //Internal variables
    uint32_t udpPacketSize;
    uint16_t numberOfParallelStreams;
    uint8_t  measureOneWay;
    uint64_t bandwidth;
    uint8_t  testAccordingToTime;
    double   durationInSeconds;
    double   printResultsInterval;

    //Server ip/port
    uint16_t serverPort;
    const char* serverIp;

    //Buffers
    uint8_t* tcpbuffer;

    //Sockets
    int socketTcpId;

    //Bind , accept variables
    struct sockaddr_in serverToConnect;

    //File descriptors set
    int maxFd;
    fd_set readDescriptors;
    fd_set writeDescriptors;

    FILE* resultsFile;
    
    //State
    bool stopRunning = false;

    //Client socket/port inforamtions
    std::vector<uint16_t> serverOpenPorts;
    std::vector<int> openSockets;
    std::vector<struct sockaddr_in> addressedToSendData;
    std::vector<ClientStreamParams*> totalParams;
    std::vector<std::thread*> openStreams;

public:
    // ======================================================================================================================================= 
    // ================================================== Constructors ======================================================================= 
    // ======================================================================================================================================= 

    ~Client();
    
    Client();

    Client(uint16_t _port);

    Client(const char* _ip);

    Client(uint16_t _port , const char* _ip);

    // ======================================================================================================================================= 
    // ================================================== Setup/Clean ======================================================================== 
    // ======================================================================================================================================= 

    void Setup();

    void SetVariables(uint32_t _udpPacketSize,
                      uint64_t _bandwidth,
                      uint16_t _numberOfParallelStreams,
                      double   _durationInSeconds,
                      uint8_t  _testAccordingToTime,
                      uint8_t  _measureOneWay,
                      uint8_t  _printInFile,
                      std::string _resultsFileName,
                      double _printResultsInterval,
                      uint8_t _printResultInter);

    void CleanUp();

    // ======================================================================================================================================= 
    // ================================================== Create Functions =================================================================== 
    // ======================================================================================================================================= 
    
    void CreateTcpClient();
   
    ClientStreamParams* CreateUdpClient(uint16_t serverOpenPort);

    void CreateStream(uint16_t serverOpenPort);

    // ======================================================================================================================================= 
    // ==================================================== TCP functions ==================================================================== 
    // =======================================================================================================================================

    void TCPSend(NerfPacket& packet);

    void TCPRecv();
    
    void ParsePacket(NerfPacket& packet);

    // ======================================================================================================================================= 
    // =======================================================Signals========================================================================= 
    // ======================================================================================================================================= 
    
    void SendLastPacketSingal();

    void SendTerminateSignal();
    
    void SendStartSignal();

    // ======================================================================================================================================= 
    // ======================================================= Run =========================================================================== 
    // ======================================================================================================================================= 
    
    void Run();

    // ======================================================================================================================================= 
    // ==================================================== Print FUnctions ==================================================================
    // =======================================================================================================================================

    void PrintResults(double throughtput,
                      double goodput,
                      double packetLost, 
                      double jitter, 
                      double jitterDeviation);

    void PrintResults(double oneWayDelay);
};

#endif 