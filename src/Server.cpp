#include "Server.h"

#include <assert.h>

// ======================================================================================================================================= 
// ================================================== Constructors ======================================================================= 
// ======================================================================================================================================= 

Server::~Server() 
{
    CleanUp();
};

Server::Server() 
{
    Setup();
};

Server::Server(uint16_t _port)
{
    Setup();
    port = _port;
};

Server::Server(const char* _ip)
{
    Setup();
    ip = _ip;
};

Server::Server(uint16_t _port , const char* _ip)
{
    Setup();
    port = _port;
    ip   = _ip;
};

// ======================================================================================================================================= 
// ================================================== Setup/Clean ======================================================================== 
// ======================================================================================================================================= 

void Server::Setup()
{
    port = DEFAULT_PORT_SERVER;
    ip   = NULL;

    socketTcpId = -1;

    tcpbuffer = new uint8_t[NERF_PACKET_SIZE];

    measurements = new Measurements();

    printInFile               = DEFAULT_PRINT_IN_FILE;
    printResultAccordingTime  = 0;
    printResultsInterval      = 0.0f;

    stopRunning = false;

    resultsFile = stdout;

    Reset();
}

void Server::Reset()
{   
    for(int socket : openSockets)
        if(socket >= 0)
            close(socket);
    openSockets.clear();
    
    for(auto param : totalParams)
    {
        delete param->measurements;
        delete param;
    }    
    totalParams.clear();
    
    for(auto stream : openStreams)
        delete stream;
    openStreams.clear();

    openPorts.clear();
    bindUdpAddresses.clear();
    
    startPrintData  = false;

    measurements->Reset();

    //internal state
    udpPacketSize           = DEFAULT_UDP_PACKET_SIZE;
    measureOneWay           = DEFAULT_MEASURE_ONE_WAY;
    numberOfParallelStreams = DEFAULT_NUMBER_OF_PARALLEL_STREAMS;
    
    //reset the timers
    printResultAccordingTimeClient  = 0;
    clientPrintResultsInterval      = 0.0f;
    clientTotalPrintResultsInterval = 0.0f;
    
    totalPrintResultsInterval      = printResultsInterval;

    memset(tcpbuffer , 0 , NERF_PACKET_SIZE);

    maxFd = -1;
    
    isClientStop    = false;

    FD_ZERO(&readDescriptors);
}

void Server::CleanUp()
{
    if(socketTcpId >= 0)
        close(socketTcpId);
    if(connectedClient >= 0)
        close(connectedClient);

    for(int socket : openSockets)
        if(socket >= 0)
            close(socket);
    openSockets.clear();
    
    for(auto param : totalParams)
    {
        delete param->measurements;
        delete param;
    }    
    totalParams.clear();
    
    for(auto stream : openStreams)
        delete stream;
    openStreams.clear();

    openSockets.clear();
    openPorts.clear();
    bindUdpAddresses.clear();

    if(tcpbuffer)
        delete [] tcpbuffer;

    if(measurements)
        delete measurements;

    FD_ZERO(&readDescriptors);

    fclose(resultsFile);
}

void Server::SetVariables(uint8_t _printInFile, 
                          std::string _resultsFileName,
                          uint8_t _printResultAccordingTime,
                          double _printResultsInterval)
{
    if(_printResultAccordingTime)
    {
        printResultsInterval      = _printResultsInterval;
        totalPrintResultsInterval = _printResultsInterval;
        printResultAccordingTime  = _printResultAccordingTime;
    }

    if(_printInFile)
    {
        printInFile = _printInFile;

        resultsFile = fopen(_resultsFileName.c_str() , "a+");
        if(!resultsFile)
        {
            fprintf(stderr, "[SERVER ~ ERROR] : unable to open file with name : %s .\n", _resultsFileName.c_str());
            return;
        }
    } 
}

void Server::StopRunning()      
{ 
    stopRunning = true;

    CleanUp();
};

// ======================================================================================================================================= 
// ================================================== Create Functions =================================================================== 
// ======================================================================================================================================= 

void Server::CreateTcpServer()
{
    if( (socketTcpId = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP)) == -1 )
    {
        perror("[TCP SERVER ~ ERROR]");
        exit(0);
    }

    memset(&bindTcpPort, 0 , sizeof(struct sockaddr_in));

    bindTcpPort.sin_family = AF_INET;
    bindTcpPort.sin_port   = htons(port);
    if(ip)
        bindTcpPort.sin_addr.s_addr = inet_addr(ip);
    else 
        bindTcpPort.sin_addr.s_addr = htonl(DEFAULT_IP_SERVER);

    if( bind(socketTcpId , (struct sockaddr*)&bindTcpPort , sizeof(struct sockaddr_in)) == -1 )
    {
        perror("[TCP SERVER ~ ERROR]");
        exit(0);
    }

    if(listen(socketTcpId , 20))
    {
        perror("[TCP SERVER ~ ERROR]");
        exit(0);
    }
}

ServerStreamParams* Server::CreateUdpServer(uint16_t portNo)
{
    int socketId;
    struct sockaddr_in bindUdpPort;

    ServerStreamParams*params = new ServerStreamParams();

    if( (socketId = socket(AF_INET , SOCK_DGRAM , 0)) == -1 )
    {
        perror("[UDP SERVER ~ ERROR]");
        exit(0);
    }

    memset(&bindUdpPort, 0 , sizeof(struct sockaddr_in));

    bindUdpPort.sin_family = AF_INET;
    bindUdpPort.sin_port   = htons(portNo);
    if(ip)
        bindUdpPort.sin_addr.s_addr = inet_addr(ip);
    else 
        bindUdpPort.sin_addr.s_addr = htonl(DEFAULT_IP_SERVER);

    if( bind(socketId , (struct sockaddr*)&bindUdpPort , sizeof(struct sockaddr_in)) == -1 )
    {
        perror("[UDP SERVER ~ ERROR]");
        exit(0);
    }

    params->socketId      = socketId;
    params->port          = portNo;
    params->udpPacketSize = udpPacketSize;
    params->udpSeqNumber  = 0;
    params->measureOneWay = measureOneWay;
    params->measurements  = new Measurements();

    openSockets.push_back(socketId);
    bindUdpAddresses.push_back(bindUdpPort);
    totalParams.push_back(params);

    return params;
}

void Server::CreateStream(uint16_t portNo)
{
    ServerStreamParams* params = CreateUdpServer(portNo);

    auto receiverHandler = [this](ServerStreamParams* params)
    {
        fd_set readDescriptors;
        int maxFd = params->socketId;

        struct sockaddr_in udpClientAddr;

        memset(&udpClientAddr , 0 , sizeof(struct sockaddr_in));

        uint32_t sockAddrinLen = sockAddrinLen = sizeof(struct sockaddr_in);

        //This is for jitter
        Time arriveTime;
        Time sendTime;
        Time diff;
        double latency;
        double prevLatency;

        std::size_t padding;    
        uint64_t    nowPacket;

        int64_t  recvLen;
        uint8_t  udpBuffer[params->udpPacketSize];

        auto UDPRecv = [&](int socketId)
        {
            recvLen = recvfrom(socketId, udpBuffer, params->udpPacketSize, 0 , (struct sockaddr*)&udpClientAddr, &sockAddrinLen);
            
            //Something went wrong with the recvfrom
            if(recvLen <= 0)
                fprintf(stderr, "[UDP SERVER ~ ERROR] : failed while trying to receive some data!\n");
            else
            {
                SystemClock::GetSystemTime(&arriveTime);
                
                if(!params->udpSeqNumber)
                    params->startTime = arriveTime;

                diff = SystemClock::GetElapsedTime(&params->startTime , &arriveTime);
                params->measurements->timeUntilNow = SystemClock::GetTimeInSeconds(&diff);                

                memcpy(&nowPacket, udpBuffer, sizeof(uint64_t));
                nowPacket = reverseBytes(nowPacket);
                
                SystemClock::Derialize(&sendTime , udpBuffer, sizeof(uint64_t));

                diff    = SystemClock::GetElapsedTime(&sendTime , &arriveTime);
                latency = SystemClock::GetTimeInSeconds(&diff);
                
                params->measurements->totalPackets++;

                if(!params->measureOneWay)
                {
                    //Throughput and goodput
                    params->measurements->totalBytesReceived    += recvLen;
                    params->measurements->totalBytesReceivedWll += (recvLen + HEADERS_FROM_THE_LAYERS);

                    params->measurements->averageThroughtput = (((params->measurements->totalBytesReceivedWll * 8) / params->measurements->timeUntilNow) / 1000000.0);
                    params->measurements->averageGoodput     = (((params->measurements->totalBytesReceived * 8) / params->measurements->timeUntilNow) / 1000000.0);
                    
                    //Find jitter
                    //We calculate the jitter using the RTP protocol formula
                    double jitter;
                    double dt;
                    if( (dt = latency - prevLatency) < 0 )
                        dt = -dt; 
                    
                    prevLatency = latency;
                    jitter      = (dt - params->measurements->jitter);

                    params->measurements->jitter += jitter / 16.0;

                    params->measurements->PushJitter(jitter);

                    //Find packets lost
                    if(nowPacket >= (params->udpSeqNumber + 1))
                    {
                        //We have lost some packets.
                        if(nowPacket > (params->udpSeqNumber + 1))
                            params->measurements->packetLost += (nowPacket - params->udpSeqNumber - 1);
                        
                        params->udpSeqNumber = nowPacket;
                        params->measurements->totalPacketsThatTheClientHaveSend = nowPacket;
                    }else
                    {
                        //We see a packet that came out of order.
                        if(params->measurements->packetLost > 0)
                            params->measurements->packetLost--;
                    }
                }
                else
                {   
                    //We assume that the one way delay is RTT/2 which is equal with the time 
                    //that the packet spend to came here (client --> server).
                    params->measurements->oneWayDelay = latency;
                }
            }
        };

        SystemClock::GetSystemTime(&params->startTime);
        while(!this->isClientStop && !this->stopRunning)
        {
            struct timeval timeout;
            timeout.tv_sec  = 1;
            timeout.tv_usec = 0; 

            FD_ZERO(&readDescriptors);

            FD_SET(params->socketId , &readDescriptors);
            int select_val = select(maxFd + 1, &readDescriptors , NULL , NULL, &timeout);
            if(select_val < 0)
            {
                perror("[UDP SERVER (STREAM) ~ INFO] : ");
                return;
            }else if(select_val == 0)
                continue;

            if(FD_ISSET(params->socketId, &readDescriptors))
                UDPRecv(params->socketId);
        }

        return;
    };

    std::thread* newStream = new std::thread(receiverHandler , params);
    openStreams.push_back(newStream);
}

// ======================================================================================================================================= 
// ==================================================== TCP functions ==================================================================== 
// =======================================================================================================================================

void Server::TCPSend(NerfPacket& packet)
{
    uint8_t buffer[NERF_PACKET_SIZE];
    size_t sendBytes;

    packet.Serialize(buffer);

    sendBytes = send(connectedClient , buffer , NERF_PACKET_IN_BYTES , 0);
}

void Server::TCPRecv()
{
    int64_t recvLen;

    recvLen = recv(connectedClient, tcpbuffer, NERF_PACKET_IN_BYTES, 0);

    NerfPacket packet = NerfPacket::Deserialize(tcpbuffer);
    ParsePacket(packet);
}

void Server::ParsePacket(NerfPacket& packet)
{
    switch(packet.flags)
    {
        case START:
        {
            startPrintData = true;

            SystemClock::GetSystemTime(&startTestTime);
        }break;

        case SETUP:
        {
            memcpy(&udpPacketSize, packet.payload, sizeof(uint32_t));
            memcpy(&numberOfParallelStreams, packet.payload + sizeof(uint32_t), sizeof(uint16_t));
            memcpy(&measureOneWay, packet.payload + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));
            memcpy(&clientPrintResultsInterval, 
                    packet.payload + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), 
                    sizeof(double));
            memcpy(&printResultAccordingTimeClient, 
                    packet.payload + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(double), 
                    sizeof(uint8_t));

            clientTotalPrintResultsInterval = clientPrintResultsInterval;

            if(!numberOfParallelStreams)
                numberOfParallelStreams++;
            
            for(uint16_t ports = 0; ports < numberOfParallelStreams; ports++)
                openPorts.push_back(++port);

            NerfPacket ports = NerfPacket::MakePortNumberPacket(openPorts);

            TCPSend(ports);
        }break;
    
        case LAST_PACKET:
        {
            uint64_t lastUdpSeqNumber;
            uint64_t currenrUdpSeqNumber;
            uint16_t port;
            
            memcpy(&port, packet.payload, sizeof(uint16_t));
            memcpy(&lastUdpSeqNumber, packet.payload + sizeof(uint16_t), sizeof(uint64_t));
            
            for(auto stream : totalParams)
            {
                if(stream->port == port)
                {
                    currenrUdpSeqNumber = stream->udpSeqNumber;
                    if(currenrUdpSeqNumber > lastUdpSeqNumber)
                        stream->measurements->packetLost -= (currenrUdpSeqNumber - lastUdpSeqNumber);
                    
                    stream->measurements->totalPacketsThatTheClientHaveSend = lastUdpSeqNumber;
                    break;
                }
            }
        }break;

        case CLOSE:
        {
            //Remove Client
            if(!isClientStop)
                isClientStop = true;

            SendMeasurements();
        }break;

        default:
        {
        }break;
    }
}

void Server::GetMeasurementsForEachStream()
{
    assert(totalParams.size() > 0);

    uint32_t streamsSize = totalParams.size();

    //copy the measurements
    memcpy(measurements , totalParams[0]->measurements , sizeof(Measurements));

    for(int stream = 1; stream < streamsSize; stream++)
    {
        if(!measureOneWay)
        {
            Measurements::CombineThroughtputs(measurements , totalParams[stream]->measurements);
            Measurements::CombineGoodputs(measurements , totalParams[stream]->measurements);
            Measurements::CombinePacketLost(measurements , totalParams[stream]->measurements);
            Measurements::CombineJitters(measurements , totalParams[stream]->measurements);
            Measurements::CombineJittersDeviations(measurements , totalParams[stream]->measurements);
        }
        else
            Measurements::CombineOneWayDelay(measurements , totalParams[stream]->measurements);     
    }
}

void Server::SendMeasurements()
{
    NerfPacket measurementsToSend;

    //Cobine the measurements for each stream
    GetMeasurementsForEachStream();

    if(!measureOneWay)
    {
        measurementsToSend = NerfPacket::MakeMeasurementsPacket(0,
                                                                measurements->GetThroughtput(),
                                                                measurements->GetGoodput(),
                                                                measurements->GetPacketLostPercentage(), 
                                                                measurements->GetJitter(), 
                                                                measurements->GetJitterStandardDeviation());
    }
    else                                                                     
    {
        measurementsToSend = NerfPacket::MakeMeasurementsPacket(1 , measurements->GetOneWayDelay());
    }

    TCPSend(measurementsToSend);
}

// ======================================================================================================================================= 
// ======================================================= Run =========================================================================== 
// ======================================================================================================================================= 

void Server::RunServer()
{
    int64_t recvLen;
    uint32_t nowPacket;

    for(auto port : openPorts)
        CreateStream(port);

    auto tcpHandler = [this]()
    {   
        double duration = 0.0f;

        while(!this->isClientStop && !this->stopRunning)
        {
            struct timeval timeout;
            timeout.tv_sec  = 1;
            timeout.tv_usec = 0; 
            
            if(startPrintData)
            {
                SystemClock::GetSystemTime(&nowTime);

                diff = SystemClock::GetElapsedTime(&startTestTime , &nowTime);
                duration = SystemClock::GetTimeInSeconds(&diff);
                
                // 0 == print results in the end
                if(printResultAccordingTimeClient && duration >= clientTotalPrintResultsInterval)
                {
                    clientTotalPrintResultsInterval += clientPrintResultsInterval;
                    SendMeasurements();
                }

                if(printResultAccordingTime && duration >= totalPrintResultsInterval)
                {
                    totalPrintResultsInterval += printResultsInterval;
                    PrintResults();
                }
            }

            FD_ZERO(&readDescriptors);

            FD_SET(connectedClient , &readDescriptors);
            maxFd = connectedClient;

            int select_val = select(maxFd + 1, &readDescriptors , NULL , NULL, &timeout);
            if(select_val < 0)
            {
                perror("[TCP SERVER ~ INFO] : ");
                return;
            }else if(select_val == 0)
                continue;

            if(FD_ISSET(connectedClient , &readDescriptors))
                TCPRecv();
        }
    };

    std::thread tcpThread(tcpHandler);
    tcpThread.join();

    for(auto openStream : openStreams)
        openStream->join();

    //Print the final results for the server side
    PrintResults();
}

void Server::Run()
{
    struct sockaddr_in clientAddr;
    
    uint32_t addrLen = sizeof(struct sockaddr_in);
    int64_t recvLen;

    while ( !stopRunning )
    {
        connectedClient = accept(socketTcpId , (struct sockaddr*)&clientAddr , &addrLen);
        if(connectedClient >= 0)
        {
            fprintf(stdout, "\n[TCP SERVER ~ LOG] : connection from ( %s , %d ).\n", inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));

            Reset();

            //wait until will receive the SETUP packet from the client
            TCPRecv();

            RunServer();
        }

        //close the connection 
        close(connectedClient);
    }
}

// ======================================================================================================================================= 
// ==================================================== Print FUnctions ==================================================================
// =======================================================================================================================================

void Server::PrintResults()
{
    //Compine the informations from the parallel streams
    GetMeasurementsForEachStream();

    if(!measureOneWay)
    {
        fprintf(resultsFile, "\nTotal Bytes Recv   :: %ld Bytes\n",   measurements->totalBytesReceived);
        fprintf(resultsFile, "Total Packets Recv :: %ld\n",           measurements->totalPackets);
        fprintf(resultsFile, "Throughtput        :: %0.3lfMbits/s\n", measurements->GetThroughtput());
        fprintf(resultsFile, "Goodput            :: %0.3lfMbits/s\n", measurements->GetGoodput());
        fprintf(resultsFile, "Packet Lost        :: %0.2lf%%\n",      measurements->GetPacketLostPercentage());
        fprintf(resultsFile, "Jitter             :: %0.2lfms\n",      measurements->GetJitter());
        fprintf(resultsFile, "Jitter Deviation   :: %0.6lf\n",        measurements->GetJitterStandardDeviation());
    }else 
        fprintf(resultsFile, "One Way Delay  :: %0.2lfms\n", measurements->GetOneWayDelay());
}
