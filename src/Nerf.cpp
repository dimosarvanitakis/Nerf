#include "Server.h"
#include "Client.h"

#include <signal.h>

Client* client = nullptr;
Server* server = nullptr;

bool isServer  = false;
bool isClient  = false;

void HandleSignal(int signalKind)
{
  if(signalKind == SIGINT)
  {
    if(isServer && server)
    {
      server->StopRunning();
    }
    else if(isClient && client)
    {
      fprintf(stdout , "\nStop sending data.\nWaiting for the final results...\n\n");

      client->SendTerminateSignal();
    }
  }
}

int main(int argc, char **argv)
{
  uint8_t experimentBaseOnTime = 0;
  uint8_t printInFile          = 0;
  
  std::string resultsFileName;

  uint16_t numberOfParallelStreams  = 0;
  uint32_t udpPacketSize            = 0;
  uint8_t  measureOneWay            = 0;
  uint8_t  printResultsInter        = 0;
  double   durationInSeconds        = 0;
  uint64_t bandwidth                = 0;
  double   waitDuration             = 0.0f;
  double   printResultsInterval     = 0.0f;

  uint16_t port                     = 0;
  const char *ip                    = NULL;

  signal(SIGINT , HandleSignal);

  int opt;
  while ((opt = getopt(argc, argv, "a:p:f:i:scl:b:n:t:w:dh")) != -1)
  {
    switch (opt)
    {
      case 'a':
      {
        ip = strdup(optarg);
      }break;

      case 'p':
      {
        port = strtol(optarg, NULL, 10);
      }break;

      case 'f':
      {
        printInFile     = 1;
        resultsFileName = std::string(optarg);
      }break;

      case 'i':
      {
        printResultsInterval = strtod(optarg , NULL);
        printResultsInter    = 1;
      }break;

      case 's':
      {
        if (isClient)
        {
          fprintf(stderr, "[Error] : you can not run Nerf as server and client simultaneously!\n");
          return 1;
        }
        else
          isServer = true;
      }break;

      case 'c':
      {
        if (isServer)
        {
          fprintf(stderr, "[Error] : you can not run Nerf as server and client simultaneously!\n");
          return 1;
        }
        else
          isClient = true;
      }break;

      case 'l':
      {
        if (isServer)
        {
          fprintf(stderr, "[Error] : you can not set this option while you running on server mode!\n");
          return 1;
        }

        udpPacketSize = strtol(optarg, NULL, 10);

        if(udpPacketSize < 16)
        {
          fprintf(stderr, "[Error] : Packet Size >= 16 bytes.\n");
          return 1;
        }
        
      }break;

      case 'b':
      {
        if (isServer)
        {
          fprintf(stderr, "[Error] : you can not set this option while you running on server mode!\n");
          return 1;
        }

        bandwidth = strtol(optarg, NULL, 10);
      }break;

      case 'n':
      {
        if (isServer)
        {
          fprintf(stderr, "[Error] : you can not set this option while you running on server mode!\n");
          return 1;
        }

        numberOfParallelStreams = strtol(optarg, NULL, 10);
      }break;

      case 't':
      {
        if (isServer)
        {
          fprintf(stderr, "[Error] : you can not set this option while you running on server mode!\n");
          return 1;
        }

        experimentBaseOnTime = 1;
        durationInSeconds    = strtod(optarg, NULL);
      }break;

      case 'd':
      {
        if (isServer)
        {
          fprintf(stderr, "[Error] : you can not set this option while you running on server mode!\n");
          return 1;
        }

        measureOneWay = 1;
      }break;

      case 'w':
      {
        if(isServer)
        {  
          fprintf(stderr, "[Error] : you can not set this option while you running on server mode!\n");
          return 1;
        }

        waitDuration = strtod(optarg , NULL);
      }break;

      case 'h':
      {
        PrintUsage();

        return 1;
      }break;

      case ':':
      {
        fprintf(stderr, "Option \'%c\' needs a value.\n", optopt);

        PrintUsage();

        return 1;
      }break;
      
      case '?':
      {
        fprintf(stderr, "Unknown option: \'%c\' .\n", optopt);
        
        PrintUsage();

        return 1;
      }break;
    }
  }

  if (isServer)
  {
    if(ip && !port)
      server = new Server(ip);
    else if(port && !ip)
      server = new Server(port);
    else if(port && ip)
      server = new Server(port,ip);
    else
    {
      fprintf(stdout, "[NERF ~ INFO] : the server listening in the default port %d and in all available interfaces.\n", DEFAULT_PORT_SERVER);
      server = new Server();
    }

    server->CreateTcpServer();
    server->SetVariables(printInFile , resultsFileName , printResultsInter , printResultsInterval);

    server->Run();
  }
  else if (isClient)
  {
    if(ip && !port)
      client = new Client(ip);
    else if(port && !ip)
      client = new Client(port);
    else if(port && ip) 
      client = new Client(port,ip);
    else 
    {
      fprintf(stdout, "[NERF ~ INFO] : the client will assume that we are running in local host with server ip %s and port %d\n", DEFAULT_SERVER_IP_TO_SEND,DEFAULT_SERVER_PORT_TO_SEND);
      client = new Client();
    }
    
    client->CreateTcpClient();
    client->SetVariables(udpPacketSize, 
                         bandwidth, 
                         numberOfParallelStreams, 
                         durationInSeconds, 
                         experimentBaseOnTime,
                         measureOneWay, 
                         printInFile, 
                         resultsFileName,
                         printResultsInterval,
                         printResultsInter);
   
    if(waitDuration)
    {
      Time start;
      Time end;
      Time diff;

      SystemClock::GetSystemTime(&start);
      while(1)
      {
        SystemClock::GetSystemTime(&end);
        
        diff = SystemClock::GetElapsedTime(&start , &end);
        if(SystemClock::GetTimeInSeconds(&diff) >= waitDuration)
          break;
      }
    }

    client->Run();
  }

  if(client)
    delete client;
  if(server)
    delete server;

  return 0;
}
