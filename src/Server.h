#ifndef _SERVER_H_
#define _SERVER_H_

#include "Utilities.h"
#include "NerfPacket.h"
#include "Measurements.h"

#define DEFAULT_PORT_SERVER               3742
#define DEFAULT_IP_SERVER                 INADDR_ANY

struct ServerStreamParams
{
    int socketId;
    uint16_t port;

    uint32_t udpPacketSize;
    uint64_t udpSeqNumber;
    uint8_t  measureOneWay;

    Measurements* measurements;

    Time startTime;
    Time nowTime;
};

class Server
{
private:
    //Time
    Time startTestTime;
    Time nowTime;
    Time diff;

    //state
    bool startPrintData;

    //Measurements
    Measurements* measurements;

    //Internal variables
    uint32_t udpPacketSize;
    uint16_t numberOfParallelStreams;
    uint8_t  measureOneWay;
    uint8_t  printInFile;
    uint8_t  printResultAccordingTimeClient;
    uint8_t  printResultAccordingTime;
    double   printResultsInterval;
    double   totalPrintResultsInterval; 
    double   clientPrintResultsInterval;
    double   clientTotalPrintResultsInterval; 

    //Server ip/port
    uint16_t    port;
    const char* ip;

    //Buffers
    uint8_t* tcpbuffer;
    
    //Sockets
    int socketTcpId;
    int connectedClient;

    //Bind , accept variables
    struct sockaddr_in bindTcpPort;

    //File descriptors set
    int maxFd;
    fd_set readDescriptors;

    //State
    bool stopRunning;
    bool isClientStop;

    //Files
    FILE* resultsFile;

    //Connected clients information
    std::vector<uint16_t>            openPorts;
    std::vector<int>                 openSockets;
    std::vector<struct sockaddr_in>  bindUdpAddresses;
    std::vector<ServerStreamParams*> totalParams;
    std::vector<std::thread*>        openStreams;

public:
    // ======================================================================================================================================= 
    // ================================================== Constructors ======================================================================= 
    // ======================================================================================================================================= 

    ~Server();
    
    Server();

    Server(uint16_t _port);

    Server(const char* _ip);

    Server(uint16_t _port , const char* _ip);

    // ======================================================================================================================================= 
    // ================================================== Setup/Clean ======================================================================== 
    // ======================================================================================================================================= 

    void Setup();

    void Reset();

    void CleanUp();
    
    void SetVariables(uint8_t _printInFile, 
                      std::string _resultsFileName,
                      uint8_t _printResultAccordingTime,
                      double  _printResultsInterval);

    void StopRunning();
    
    // ======================================================================================================================================= 
    // ================================================== Create Functions =================================================================== 
    // ======================================================================================================================================= 
    
    void CreateTcpServer();

    ServerStreamParams* CreateUdpServer(uint16_t portNo);

    void CreateStream(uint16_t portNo);

    // ======================================================================================================================================= 
    // ==================================================== TCP functions ==================================================================== 
    // =======================================================================================================================================
    
    void TCPSend(NerfPacket& packet);

    void TCPRecv();

    void ParsePacket(NerfPacket& packet);

    void GetMeasurementsForEachStream();

    void SendMeasurements();

    // ======================================================================================================================================= 
    // ======================================================= Run =========================================================================== 
    // ======================================================================================================================================= 

    void RunServer();

    void Run();

    // ======================================================================================================================================= 
    // ==================================================== Print FUnctions ==================================================================
    // =======================================================================================================================================

    void PrintResults();
};

#endif