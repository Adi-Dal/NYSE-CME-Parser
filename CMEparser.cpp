/*------------------
Created By Aditya Dalal
Date: 11/23/2024
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <string>
#include "DebugUtil.h"
#include "MessageHeader.h"
#include "MDInstrumentDefinitionSpread56.h"
#include "MDIncrementalRefreshOrderBook47.h"
#include "MDInstrumentDefinitionOption55.h"
#include "MDInstrumentDefinitionFuture54.h"
#include "MDIncrementalRefreshBook46.h"
#include "AdminHeartbeat12.h"
using namespace std;

class cme_parser{
    private:
        struct packet{
            mktdata::MessageHeader mh;
            char* buffer;
            int bufSize;
            int packetNum;
        };
        vector<uint8_t> bytes;
        vector<packet> packetList;
        char* packetBuffer;
        char* bufPointer;
        string inputFile;
        string outputFile;
        int batchSize = 10000;
        //functions to output template specific information
        void template12(char* buffer, uint64_t blocklength ,uint64_t bufferlength,uint64_t version,stringstream& message){
            string result = "";
            mktdata::AdminHeartbeat12 temp2;
            temp2.wrapForDecode(buffer,10,blocklength,version,bufferlength);
            message << "," << temp2;
        }
        void template46(char* buffer, uint64_t blocklength ,uint64_t bufferlength,uint64_t version,stringstream& message){
            string result = "";
            mktdata::MDIncrementalRefreshBook46 temp2;
            temp2.wrapForDecode(buffer,10,blocklength,version,bufferlength);
            mktdata::MDIncrementalRefreshBook46::NoMDEntries mbofd = temp2.noMDEntries(); 
            message << "," << temp2;
        }

        void template47(char* buffer, uint64_t blocklength ,uint64_t bufferlength,uint64_t version,stringstream& message){
            string result = "";
            mktdata::MDIncrementalRefreshOrderBook47 temp2;
            temp2.wrapForDecode(buffer,10,blocklength,version,bufferlength);
            mktdata::MDIncrementalRefreshOrderBook47::NoMDEntries mbofd = temp2.noMDEntries();
            bool hasOne = false;
            while(mbofd.hasNext()){
                mbofd.next();
                message << ",{" << mbofd.orderID() << "," << mbofd.mDOrderPriority() << "," << mbofd.mDEntryPx() << "," << mbofd.mDDisplayQty() << "," << mbofd.securityID() << "," << mbofd.mDUpdateAction() << "," << mbofd.mDEntryType()<<"}";

            }
        }
        void template54(char* buffer, uint64_t blocklength ,uint64_t bufferlength,uint64_t version,stringstream& message){
            string result = "";
            mktdata::MDInstrumentDefinitionFuture54 temp2;
            temp2.wrapForDecode(buffer,10,blocklength,version,bufferlength);
            message << "," << temp2;
        }
        void template55(char* buffer, uint64_t blocklength ,uint64_t bufferlength,uint64_t version,stringstream& message){
            string result = "";
            mktdata::MDInstrumentDefinitionOption55 temp2;
            temp2.wrapForDecode(buffer,10,blocklength,version,bufferlength);
            message << "," << temp2;
        }
        void template56(char* buffer, uint64_t blocklength ,uint64_t bufferlength,uint64_t version,stringstream& message){
            string result = "";
            mktdata::MDInstrumentDefinitionSpread56 temp2;
            temp2.wrapForDecode(buffer,10,blocklength,version,bufferlength);
            message << "," << temp2;
        }

    public:
        cme_parser(int batch, string inputFileName, string outputFileName){
            batchSize = batch;
            inputFile = inputFileName;
            outputFile = outputFileName;

            //open and store the bytes into a vector:
            ifstream input(inputFile,ios::binary);
            if(!input.is_open()){
                cout << "Error opening file " << inputFile << endl;
                //add exception handling
            }

            input.seekg(0, ios::end);
            streamsize fileSize = input.tellg();
            input.seekg(0, ios::beg);
            vector<uint8_t> tempBytes(fileSize,0);
            char byte;
            bytes.reserve(fileSize);
            //read and insert to vector
            vector<char> buffer(2048);
            while (input.read(buffer.data(), buffer.size())) {
                bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + input.gcount());
            }
            bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + input.gcount());
            //erase global header - 24 bytes
            for(int i = 0; i <24;i++){
                cout << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i] << " ";
            }
            bytes.erase(bytes.begin(),bytes.begin()+24);
            char* buf = new char[bytes.size()];
            //Assign to help keep track of allocated memory. Assign bufPointer to buf since that will be used to represent the buffer in rest of program
            packetBuffer = buf;
            bufPointer= buf;
            copy(bytes.begin(), bytes.end(), buf);
            input.close();
        }

        //destructor clear memory to prevent any memory leaks
        ~cme_parser(){
            delete packetBuffer;
        }

        void parse(){
            ofstream output(outputFile);
            if(!output.is_open()){
                cout << "Could not open file " << outputFile << endl;
                //add exception handling;
            }
            output << "BlockLength,TemplateID,SchemaID,Version,TemplateInfo" << endl;
            int byteNum = bytes.size();
            uint16_t version = 0;
            uint16_t messageSize = 0x0000;
            int msgSize = 0;
            mktdata::MessageHeader temp;
            int batchNum = 0;
            int packetNum = 1;

            while(byteNum>0){
                uint32_t pcapLength = (static_cast<uint32_t>((static_cast<uint16_t>(bytes[bytes.size()-byteNum+11])<<8) | bytes[bytes.size()-byteNum+10])<<16) | ((static_cast<uint16_t>(bytes[bytes.size()-byteNum+9])<<8) | bytes[bytes.size()-byteNum+8]);
                //remove packet headers. Has time stamps in seconds and microseconds, captured length, and original length. Total 16 bytes:
                bufPointer+=16;
                byteNum-=16;
                // //remove the ethernet,IP. and UDP headers. Total of 42 bytes:
                bufPointer+=42;
                byteNum-=42;
                //remove the technical header(12 bytes total):
                bufPointer+=12;
                byteNum-=12;
                //now at message header. Format= msgSize(2 bytes), blockLength(2 bytes), templateID(2 bytes), schemaID(2 bytes), version(2 bytes):
                version = (static_cast<uint16_t>(bufPointer[9])<<8) | bufPointer[8];
                messageSize = (static_cast<uint16_t>(bytes[bytes.size()-byteNum+1])<<8) | bytes[bytes.size()-byteNum];
                msgSize = static_cast<int>(messageSize);
                messageSize = 0x0000;
                temp.wrap(bufPointer,2,version,byteNum);
                packet tempPacket;
                tempPacket.mh = temp;
                tempPacket.buffer = bufPointer;
                tempPacket.bufSize = byteNum;
                tempPacket.packetNum = packetNum;
                int changePosition = pcapLength-42-12;
                // output << changePosition << " " << pcapLength << " " << msgSize << " " << temp.blockLength() << " " << temp.templateId() << " " << temp.schemaId() << " " << temp.version() << endl;
                //used to only output packets with templates 55 and 46
                // if(temp.templateId()==47){
                //     packetList.push_back(tempPacket);
                // }
                packetList.push_back(tempPacket);
  
                if(packetList.size()>batchSize){
                    writeToFile(packetList,output);
                    packetList.clear();
                    cout << "Batch " << batchNum << " completed" << endl;
                    batchNum++;
                }
                bufPointer+=changePosition;
                byteNum-=changePosition;
                packetNum++;
            }
            cout << "Phase 4" << endl;
            //write remaining packets to output
            if(!packetList.empty()){
                writeToFile(packetList,output);
            }
            cout << "Parsed " << packetNum << " packets." << endl;
            output.close();
        }

        void writeToFile(const vector<packet>& packList, ofstream& outputFile){
            stringstream content;
            for(const packet& packet1: packList){
                content << packet1.packetNum <<"," <<packet1.mh.blockLength()<<","<<packet1.mh.templateId()<<","<<packet1.mh.schemaId()<<","<<packet1.mh.version();
                if(packet1.mh.templateId()==47){
                    template47(packet1.buffer,packet1.mh.blockLength(),packet1.bufSize,packet1.mh.version(),content);
                }
                if(packet1.mh.templateId()==55){
                    template55(packet1.buffer,packet1.mh.blockLength(),packet1.bufSize,packet1.mh.version(),content);
                }
                else if(packet1.mh.templateId()==12){
                    template12(packet1.buffer,packet1.mh.blockLength(),packet1.bufSize,packet1.mh.version(),content);
                }
                else if(packet1.mh.templateId()==46){
                    template46(packet1.buffer,packet1.mh.blockLength(),packet1.bufSize,packet1.mh.version(),content);
                }
                else if(packet1.mh.templateId()==54){
                    template54(packet1.buffer,packet1.mh.blockLength(),packet1.bufSize,packet1.mh.version(),content);
                }
                else if(packet1.mh.templateId()==56){
                    template56(packet1.buffer,packet1.mh.blockLength(),packet1.bufSize,packet1.mh.version(),content);
                }
                content<<"\n";
            }
            outputFile << content.str();
        }
};

//use for testing the time
// int main(){
//     string in = "CME_Example.pcap";
//     string out = "cme_example100.csv";
//     cme_parser pcap(10000,in,out);
//     auto start = chrono::high_resolution_clock::now();
//     pcap.parse();
//     auto end = chrono::high_resolution_clock::now();
//     std::chrono::duration<double> struct_duration = end - start;
//     std::cout << "Time: " << struct_duration.count() << " seconds\n";
//     return 0 ;
// }
