#include <iostream>
#include <fstream>
#include <thread>
#include <unistd.h> 
#include <netinet/in.h>
#include <string.h>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <iomanip>
#include "date.h"
#include "md5.h"

using namespace std;
using namespace date;

#define DEFAULT_PORT 3500
#define MAX_DATA_LEN 1600
#define MD5_LEN 16

uint32_t global_packet_counter = 0;
int64_t timeZoneNanosecOffset = 0;
string DataFileName="/home/eugene/projects/TestProject/udpClient/Untitled2.csv";

struct DataHeader 
{
    uint32_t ID;
    u_int64_t TimeStamp;
    uint16_t DataCount;
    uint8_t Md5CheckSum[MD5_LEN];   
};

struct DataPacket
{
    DataHeader Header;
    int16_t Data[MAX_DATA_LEN];
};

long GetTimeZoneNanosecOffset()
{
    time_t when = std::time(nullptr);
    auto const tm = *std::localtime(&when);
    ostringstream os;
    os << std::put_time(&tm, "%z");
    string s = os.str();
    // s is in ISO 8601 format: "±HHMM"
    int32_t h = std::stoi(s.substr(0,3), nullptr, 10);
    int32_t m = std::stoi(s[0]+s.substr(3), nullptr, 10);
    int64_t offset = ((long)h * 3600 + (long)m * 60)*1000000000;
    return offset;
}

uint64_t GetTimeStamp()
{
    chrono::system_clock::time_point tp = chrono::system_clock::now();
    uint64_t timeStamp;
    memcpy(&timeStamp, &tp, 8);
    return timeStamp + timeZoneNanosecOffset;
}

void PrintTimeStamp(uint64_t timeStamp)
{  
    chrono::system_clock::time_point tp;
    memcpy(&tp, &timeStamp, 8);
    cout << tp;
}

bool FileExists(const std::string& path)
{
    return std::ifstream(path.c_str()).good();
}

uint16_t GetDataCountFromLine(string line, int16_t* dataBuf)
{
    int32_t dataCount = 0;
    std::stringstream lineStream(line);
    string field;
    int16_t value = 0;
    int32_t offset = 0;
    while (getline(lineStream, field,',')){
        dataCount++;
        value = stoi(field);
        if(dataCount<=MAX_DATA_LEN){               //read only MAX_DATA_LEN elements
            dataBuf[offset]=value;
            offset++;
        }   
    }
    return dataCount;
}

string GetDataString(const int16_t* dataBuf, uint16_t count)
{
    string resltStr ="";
    for(int32_t i=0; i<count; i++){
        if(i==count-1)
            resltStr+=std::to_string(dataBuf[i]); 
        else
            resltStr+=std::to_string(dataBuf[i])+",";            
    }
    return resltStr;
}

void SendDataFromFile(int32_t sock, struct sockaddr* addr, uint lineCount)
{
    DataPacket dataPacket;
    
    uint lineCounter = 0;
    string line;
    string md5_str;
    string data_str;
    uint64_t timeStamp = 0;
    uint packetSize=0;

    std::ifstream csv_file(DataFileName); 
    if (!csv_file.is_open())
    {
        cout<<"Can't open file!\n";           
    }else
    {
        label1:
        while (getline(csv_file, line))
        {
            lineCounter++;
            if(lineCounter>lineCount)
                break;
            
            timeStamp = GetTimeStamp();
            global_packet_counter++;
            dataPacket.Header.ID=global_packet_counter;
            dataPacket.Header.TimeStamp=timeStamp;
            dataPacket.Header.DataCount=GetDataCountFromLine(line,dataPacket.Data);

            if(dataPacket.Header.DataCount>MAX_DATA_LEN){
                cout<<"Line DataCount>"<<std::to_string(MAX_DATA_LEN)<<" Skip this line\n";
                global_packet_counter--;
                continue;  
            }

            MD5 md5 = MD5(line);
            memcpy(dataPacket.Header.Md5CheckSum,md5.digest,16);
            md5_str=md5.hexdigest();

            packetSize=sizeof(DataHeader)+sizeof(dataPacket.Data[0])*dataPacket.Header.DataCount;

            sendto(sock, (const void*)&dataPacket, packetSize, 0, addr, sizeof(*addr));     // Send packet
            cout<<"Sent: #"<<dataPacket.Header.ID << " ";
            PrintTimeStamp(dataPacket.Header.TimeStamp);
            
            cout<<" md5="<< md5_str;
            //data_str=GetDataString(dataPacket.Data,dataPacket.Header.DataCount);          // print data for debug
            //cout<<" data="<< data_str;
            cout<<endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));                     // Wait 10 milliseconds
        }
        if(lineCounter<lineCount){
            csv_file.clear();
            csv_file.seekg(0);
            goto label1;
        }
    }
    csv_file.close();     
}

int main()
{       
    if(!FileExists(DataFileName)){
        cout << "Data file not found. Close the program\n";
        return 1;
    }
    
    timeZoneNanosecOffset = GetTimeZoneNanosecOffset();

    int32_t sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0){
        cout << "ERROR Create socket. Close program\n";
        return 2;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    cout << "UdpClient. Start sending packets\n";

    SendDataFromFile(sock,(struct sockaddr *)&addr,100);  
    std::this_thread::sleep_for(std::chrono::seconds(10)); 
    SendDataFromFile(sock,(struct sockaddr *)&addr,100);
   
    close(sock);

    cout << "UdpClient. All packets was sended. Total count: " << global_packet_counter << endl;
    return 0;
}
