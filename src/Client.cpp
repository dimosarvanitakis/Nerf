#include "Client.h"

// ======================================================================================================================================= 
// ================================================== Constructors ======================================================================= 
// ======================================================================================================================================= 

Client::~Client() 
{
    CleanUp();
};

Client::Client() 
{
    Setup();
};

Client::Client(uint16_t _port)
{
    Setup();
    serverPort = _port;
};

Client::Client(const char* _ip)
{
    Setup();
    serverIp = _ip;
};

Client::Client(uint16_t _port , const char* _ip)
{
    Setup();
    serverPort = _port;
    serverIp   = _ip;
}

// ======================================================================================================================================= 
// ================================================== Setup/Clean ======================================================================== 
// ======================================================================================================================================= 

void Client::Setup()
{
    udpPacketSize             = DEFAULT_UDP_PACKET_SIZE;
    numberOfParallelStreams   = DEFAULT_NUMBER_OF_PARALLEL_STREAMS;
    measureOneWay             = DEFAULT_MEASURE_ONE_WAY;
    bandwidth                 = DEFAULT_BANDWIDTH;
    printResultsInterval      = DEFAULT_INTERVAL_TO_PRINT;
    testAccordingToTime       = 0;
    durationInSeconds         = 0;

    serverPort = DEFAULT_SERVER_PORT_TO_SEND;
    serverIp   = NULL;

    tcpbuffer = new uint8_t[NERF_PACKET_SIZE];
    memset(tcpbuffer , 0 , NERF_PACKET_SIZE);

    socketTcpId = -1;

    maxFd = -1;
    FD_ZERO(&readDescriptors);

    resultsFile = stdout;

    stopRunning = false;
}

void Client::SetVariables(uint32_t _udpPacketSize,
                          uint64_t _bandwidth,
                          uint16_t _numberOfParallelStreams,
                          double   _durationInSeconds,
                          uint8_t  _testAccordingToTime,
                          uint8_t  _measureOneWay,
                          uint8_t  _printInFile,
                          std::string _resultsFileName,
                          double _printResultsInterval,
                          uint8_t _printResultInter)
{
    if(_udpPacketSize) 
        udpPacketSize = _udpPacketSize;
    
    if(_bandwidth)
        bandwidth = _bandwidth;
    
    if(_numberOfParallelStreams)
        numberOfParallelStreams = _numberOfParallelStreams;
    
    if(_durationInSeconds)
    {
        durationInSeconds   = _durationInSeconds;
        testAccordingToTime = _testAccordingToTime;
    }

    if(_measureOneWay) 
        measureOneWay = _measureOneWay;
    
    if(_printInFile)
    {   
        resultsFile = fopen(_resultsFileName.c_str() , "a+");
        if(!resultsFile)
        {
            fprintf(stderr, "[CLIENT ~ ERROR] : unable to open file with name : %s .\n", _resultsFileName.c_str());
            return;
        }
    }

    if(_printResultsInterval)
        printResultsInterval = _printResultsInterval;

    //Send the "setup" parameters to the server
    NerfPacket setupPacket = NerfPacket::MakeSetupPacket(udpPacketSize,numberOfParallelStreams,measureOneWay,printResultsInterval,_printResultInter);
    TCPSend(setupPacket);
    //Wait to recv the open ports that the client create
    TCPRecv();
}

void Client::CleanUp()
{
    if(socketTcpId > 0)
        close(socketTcpId);
    
    for(auto socket : openSockets)
        if(socket >= 0)
            close(socket);
    openSockets.clear();

    for(auto params : totalParams)
        delete params;
    totalParams.clear();

    for(auto thread : openStreams)
        delete thread;
    openStreams.clear();
    
    addressedToSendData.clear();
    serverOpenPorts.clear();

    if(tcpbuffer)
        delete [] tcpbuffer;
    
    if(resultsFile)
        fclose(resultsFile);

    FD_ZERO(&readDescriptors);
    FD_ZERO(&writeDescriptors);
}

// ======================================================================================================================================= 
// ================================================== Create Functions =================================================================== 
// ======================================================================================================================================= 

void Client::CreateTcpClient()
{
    if( (socketTcpId = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP)) == -1 )
    {
        perror("[TCP CLIENT ~ ERROR]");
        exit(0);
    }

    memset(&serverToConnect, 0 , sizeof(struct sockaddr_in));

    serverToConnect.sin_family = AF_INET;
    serverToConnect.sin_port   = htons(serverPort);
    if(serverIp)
        serverToConnect.sin_addr.s_addr = inet_addr(serverIp);
    else 
        serverToConnect.sin_addr.s_addr = inet_addr(DEFAULT_SERVER_IP_TO_SEND);
    
    if( connect(socketTcpId , (struct sockaddr*)&serverToConnect, sizeof(struct sockaddr_in)) )
    {
        perror("[TCP CLIENT ~ ERROR]");
        exit(0);
    }
}

ClientStreamParams* Client::CreateUdpClient(uint16_t serverOpenPort)
{
    ClientStreamParams* params = new ClientStreamParams();

    int socketId;
    struct sockaddr_in serverToSendUpdData;

    if( (socketId = socket(AF_INET , SOCK_DGRAM , 0)) == -1 )
    {
        perror("[UDP CLIENT ~ ERROR]");
        exit(0);
    }

    memset(&serverToSendUpdData , 0 , sizeof(struct sockaddr_in));

    serverToSendUpdData.sin_family = AF_INET;
    serverToSendUpdData.sin_port   = htons(serverOpenPort);
    if(serverIp)
        serverToSendUpdData.sin_addr.s_addr = inet_addr(serverIp);
    else 
        serverToSendUpdData.sin_addr.s_addr = inet_addr(DEFAULT_SERVER_IP_TO_SEND);

    params->socketId          = socketId;
    params->port              = serverOpenPort;
    params->bandwidth         = bandwidth;
    params->durationInSeconds = durationInSeconds;
    params->serverToSendData  = serverToSendUpdData;
    params->stop              = false;
    params->udpPacketSize     = udpPacketSize;
    params->udpSeqNumber      = 0;
    params->totalBytesSend    = 0;

    addressedToSendData.push_back(serverToSendUpdData);
    openSockets.push_back(socketId);
    totalParams.push_back(params);

    return params;
}

void Client::CreateStream(uint16_t serverOpenPort)
{
    auto senderHandler = [](ClientStreamParams* params)
    {
        uint8_t  udpBuffer[params->udpPacketSize];

        Time bandwidthTimeBegin;
        Time bandwidthTimeEnd;

        Time stopTestBegin;
        Time stopTestEnd;
        Time sendTime;
        Time diff;

        uint64_t numberOfPacketsToSend = ((params->bandwidth / 8) / params->udpPacketSize);
        uint32_t remainingBytesToSend  = ((params->bandwidth / 8) - (numberOfPacketsToSend * params->udpPacketSize));

        auto UDPSend = [&](uint32_t bytesToSend)
        {
            int64_t     bytesSend;

            params->udpSeqNumber++;
            
            uint64_t sendSeq = reverseBytes(params->udpSeqNumber);
            memcpy(udpBuffer, &sendSeq , sizeof(uint64_t));
            
            SystemClock::GetSystemTime(&sendTime);
            SystemClock::Serialize(&sendTime, udpBuffer, sizeof(uint64_t));

            bytesSend = sendto(params->socketId , udpBuffer , bytesToSend , 0 , (struct sockaddr*)&params->serverToSendData, sizeof(struct sockaddr_in));
            if(bytesSend <= 0)
                fprintf(stderr, "[UDP CLIENT ~ ERROR] : Something went wrong while trying to send data!\n");
            else 
                params->totalBytesSend += bytesSend; 
        };

        auto CheckTime = [&]()-> bool 
        {
            if(params->durationInSeconds)
            {
                SystemClock::GetSystemTime(&stopTestEnd);
                
                diff = SystemClock::GetElapsedTime(&stopTestBegin , &stopTestEnd);
            
                if(SystemClock::GetTimeInSeconds(&diff) >= params->durationInSeconds)
                    return true;
            }
            return false;
        };

        SystemClock::GetSystemTime(&stopTestBegin);
        while(1)
        {
            if(params->stop)
                return;

            uint64_t sleepTime;
            uint64_t nano;
            
            SystemClock::GetSystemTime(&bandwidthTimeBegin);
            for(uint64_t packet = 0; packet < numberOfPacketsToSend; packet++)
            {
                if(CheckTime())
                    return;
                UDPSend(params->udpPacketSize);
            }

            if(remainingBytesToSend)
                UDPSend(remainingBytesToSend);
            SystemClock::GetSystemTime(&bandwidthTimeEnd);

            diff      = SystemClock::GetElapsedTime(&bandwidthTimeBegin , &bandwidthTimeEnd);
            nano      = SystemClock::GetTimeInNanoSeconds(&diff); 
            sleepTime = ONE_SECOND_TO_NANO - nano;

            if(sleepTime > 0)
                std::this_thread::sleep_for(std::chrono::nanoseconds(sleepTime));
        }
    };
    
    ClientStreamParams* params = CreateUdpClient(serverOpenPort);
    
    openStreams.push_back(new std::thread(senderHandler , params));
}

// ======================================================================================================================================= 
// ==================================================== TCP functions ==================================================================== 
// ======================================================================================================================================= 

void Client::TCPSend(NerfPacket& packet)
{
    size_t sendBytes;

    packet.Serialize(tcpbuffer);

    sendBytes = send(socketTcpId , tcpbuffer , NERF_PACKET_IN_BYTES , 0);
}

void Client::TCPRecv()
{
    int64_t recvLen;

    recvLen = recv(socketTcpId, tcpbuffer, NERF_PACKET_IN_BYTES, 0);

    NerfPacket packet = NerfPacket::Deserialize(tcpbuffer);
    ParsePacket(packet);
}

void Client::ParsePacket(NerfPacket& packet)
{
    if(packet.flags == MEASUREMENT)
    {
        uint8_t isOneWay;
    
        memcpy(&isOneWay, packet.payload, sizeof(uint8_t));
        if(isOneWay)
        {
            double oneWayDelay;
            memcpy(&oneWayDelay, packet.payload + sizeof(uint8_t), sizeof(double));

            PrintResults(oneWayDelay);
            return;
        }

        double  averageThroughput;
        double  averageGoodput;
        double  jitter;
        double  jitterDeviation;
        double  packetLost;

        memcpy(&averageThroughput ,packet.payload + sizeof(uint8_t)                         ,sizeof(double));
        memcpy(&averageGoodput    ,packet.payload + sizeof(uint8_t) + sizeof(double)        ,sizeof(double));
        memcpy(&packetLost        ,packet.payload + sizeof(uint8_t) + (2 * sizeof(double))  ,sizeof(double));
        memcpy(&jitter            ,packet.payload + sizeof(uint8_t) + (3 * sizeof(double))  ,sizeof(double));
        memcpy(&jitterDeviation   ,packet.payload + sizeof(uint8_t) + (4 * sizeof(double))  ,sizeof(double));

        PrintResults(averageThroughput, averageGoodput, packetLost , jitter , jitterDeviation);
    }
    else if(packet.flags == OPEN_PORTS)
    {
        uint32_t numberOfPorts;

        memcpy(&numberOfPorts, packet.payload, sizeof(uint32_t));
        for(uint32_t ports = 0; ports < numberOfPorts; ports++)
        {
            uint16_t port;
            memcpy(&port, packet.payload + sizeof(uint32_t) + (ports * sizeof(uint16_t)), sizeof(uint16_t));
            serverOpenPorts.push_back(port);
        }
    }
}
 
// ======================================================================================================================================= 
// =======================================================Signals========================================================================= 
// ======================================================================================================================================= 

void Client::SendLastPacketSingal()
{
    //Inform the server for the last packet that each stream has send
    for(auto stream : totalParams)
    {
        NerfPacket packet = NerfPacket::MakeLastSequenceNumberPacket(stream->port , stream->udpSeqNumber);

        TCPSend(packet);
    }
}

void Client::SendTerminateSignal()
{
    //Notify the parallel streams to STOP
    stopRunning = true;

    for(auto params : totalParams)
        params->stop = true;

    NerfPacket cancel = NerfPacket::MakeClosePacket();

    SendLastPacketSingal();        

    //inform the server that we are going to close the connection.
    //Now the server must responce with an "measurements" packet. 
    TCPSend(cancel);

    //wait until we get the measurements from the server
    TCPRecv();
}

void Client::SendStartSignal()
{
    NerfPacket start = NerfPacket::MakeStartPacket();

    //We send the start packet to inforf the server that NOW will start sending UDP data.
    TCPSend(start);
}

// ======================================================================================================================================= 
// ======================================================= Run =========================================================================== 
// ======================================================================================================================================= 

void Client::Run()
{  
    for(auto port : serverOpenPorts)
        CreateStream(port);

    auto tcpHandle = [this]()
    {    
        Time stopTestBegin;
        Time stopTestEnd;
        Time diff;

        SendStartSignal();

        SystemClock::GetSystemTime(&stopTestBegin);
        while(!this->stopRunning)
        {
            struct timeval timeout;
            timeout.tv_sec  = 1;
            timeout.tv_usec = 0; 

            if(this->testAccordingToTime)
            {
                SystemClock::GetSystemTime(&stopTestEnd);
                
                diff = SystemClock::GetElapsedTime(&stopTestBegin , &stopTestEnd);
            
                if(SystemClock::GetTimeInSeconds(&diff) >= durationInSeconds)
                    return;
            }

            FD_ZERO(&readDescriptors);
            FD_SET(socketTcpId , &readDescriptors);

            maxFd = socketTcpId;

            int select_val = select(maxFd + 1, &readDescriptors , NULL , NULL, &timeout);
            if(select_val < 0)
            {
                perror("[TCP SERVER ~ INFO] : ");
                return;
            }else if(select_val == 0)
                continue;

            if(FD_ISSET(socketTcpId , &readDescriptors) && !this->stopRunning)
                TCPRecv();
        }

        return;
    };

    std::thread tcpThread(tcpHandle);
    tcpThread.join();

    for(auto thread : openStreams)
        thread->join();

    if(testAccordingToTime && !stopRunning)
    {
        fprintf(stdout , "\nStop sending data.\nWaiting for the final results...\n\n");
        
        SendTerminateSignal();
    }

    return;
}

// ======================================================================================================================================= 
// ==================================================== Print Functions ==================================================================
// =======================================================================================================================================


void Client::PrintResults(double throughtput,
                          double goodput,
                          double packetLost, 
                          double jitter, 
                          double jitterDeviation)
{
    uint64_t totalPacketsSend = 0;
    int64_t  totalBytesSend   = 0;

    for(auto params : totalParams)
    {
        totalPacketsSend += params->udpSeqNumber;
        totalBytesSend   += params->totalBytesSend;
    }

    fprintf(resultsFile, "\nTotal Bytes Send   :: %ld Bytes\n",     totalBytesSend);
    fprintf(resultsFile, "Total Packets Send :: %ld\n",             totalPacketsSend);
    fprintf(resultsFile, "Throughtput        :: %0.3lfMbits/s\n",   throughtput);
    fprintf(resultsFile, "Goodput            :: %0.3lfMbits/s\n",   goodput);
    fprintf(resultsFile, "Packet Lost        :: %0.2lf%%\n",        packetLost);
    fprintf(resultsFile, "Jitter             :: %0.2lfms\n",        jitter);
    fprintf(resultsFile, "Jitter Deviation   :: %0.6lf\n",          jitterDeviation);
}

void Client::PrintResults(double oneWayDelay)
{
    fprintf(resultsFile, "One Way Delay  :: %0.2lfms\n", oneWayDelay);
}