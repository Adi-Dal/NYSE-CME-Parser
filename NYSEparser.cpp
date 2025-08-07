Created By Aditya Dalal & Anton Charov
Date: 12/14/2024
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

using namespace std;

class nyse_parser{
    private:
        struct packHeader{
            uint32_t sendTime;
            uint32_t sendTimeNS;
            uint32_t seqNum;
            uint16_t packetSize;
            uint8_t numMsgs;
            uint8_t flag;
        };
        struct message {
            uint64_t packetCaptureTime;
            uint64_t sendTime;
            uint64_t sendTimeNS;
            unsigned char* buffer;
            uint16_t msgSize;
            uint16_t msgType;
        };
        vector<uint8_t> bytes;
        vector<message> messageList;
        unsigned char* bufPointer;
        unsigned char* origPointer;
        string input_file;
        string output_file;
        string prl_output_file;
        string trd_output_file;
        int batch_size = 10000;

        // NYSE Chicago hours: 6:30am - 8:00pm

        // functions for parsing each message type:

        // addOrderMessage
        void parse100(stringstream& content, stringstream& prl_content, stringstream& trd_content, const message& message100){
            uint32_t sourceTimeNS = static_cast<uint32_t>(message100.buffer[7] << 24) | static_cast<uint32_t>(message100.buffer[6] << 16) | static_cast<uint32_t>(message100.buffer[5] << 8) | message100.buffer[4];
            uint32_t symbolIDX = static_cast<uint32_t>(message100.buffer[11] << 24) | static_cast<uint32_t>(message100.buffer[10] << 16) | static_cast<uint32_t>(message100.buffer[9] << 8) | message100.buffer[8];
            uint32_t symbolSeqNum = static_cast<uint32_t>(message100.buffer[15] << 24) | static_cast<uint32_t>(message100.buffer[14] << 16) | static_cast<uint32_t>(message100.buffer[13] << 8) | message100.buffer[12];
            uint64_t orderID = (static_cast<uint64_t>(message100.buffer[23]) << 56) |(static_cast<uint64_t>(message100.buffer[22]) << 48) |(static_cast<uint64_t>(message100.buffer[21]) << 40) |(static_cast<uint64_t>(message100.buffer[20]) << 32) |static_cast<uint64_t>(message100.buffer[19] << 24) | static_cast<uint64_t>(message100.buffer[18] << 16) | static_cast<uint64_t>(message100.buffer[17] << 8) | message100.buffer[16];
            uint32_t price = static_cast<uint32_t>(message100.buffer[27] << 24) | static_cast<uint32_t>(message100.buffer[26] << 16) | static_cast<uint32_t>(message100.buffer[25] << 8) | message100.buffer[24];
            uint32_t volume = static_cast<uint32_t>(message100.buffer[31] << 24) | static_cast<uint32_t>(message100.buffer[30] << 16) | static_cast<uint32_t>(message100.buffer[29] << 8) | message100.buffer[28];
            char side;
            if(message100.buffer[32]==0x53) side = 'S';
            if(message100.buffer[32]==0x42) side = 'B';
            //Look into only displaying when the ascii values are not blank or null
            string firm = "";
            firm+=static_cast<char>(message100.buffer[47])+static_cast<char>(message100.buffer[36])+static_cast<char>(message100.buffer[35])+static_cast<char>(message100.buffer[34])+static_cast<char>(message100.buffer[33]);
            // Add to main content
            content << "," << sourceTimeNS << "," << symbolIDX << "," << symbolSeqNum << "," << orderID << "," << price << "," << volume << "," << static_cast<char>(message100.buffer[32]) << "," << firm;
            // Populate PRL content
            // PacketCaptureTime,SendTime,Price,Side,Symbol,Size,RecordType,OrderID,NumParticipants,EventFlag,TickType
            prl_content << sourceTimeNS << "," // PacketCaptureTime
                        << message100.sendTime << message100.sendTimeNS << "," // SendTime
                        << symbolSeqNum << ","  // MessageID
                        << sourceTimeNS << ","  // Raw Timestamp
                        << "PRL,"               // TickType
                        << symbolIDX << "," // Symbol
                        << price / 1000000.0 << "," // Price converted to float
                        << volume << ","        // Size
                        << "R,"                 // RecordType
                        << static_cast<int>(message100.buffer[0]) << "," // Flag
                        << (side == 'S' ? "1" : "0") << "\n"; // ASK
        }
        // modifyOrderMessage
        void parse101(stringstream& content, stringstream& prl_content, stringstream& trd_content, const message& message101){
            uint32_t sourceTimeNS = static_cast<uint32_t>(message101.buffer[7] << 24) | static_cast<uint32_t>(message101.buffer[6] << 16) | static_cast<uint32_t>(message101.buffer[5] << 8) | message101.buffer[4];
            uint32_t symbolIDX = static_cast<uint32_t>(message101.buffer[11] << 24) | static_cast<uint32_t>(message101.buffer[10] << 16) | static_cast<uint32_t>(message101.buffer[9] << 8) | message101.buffer[8];
            uint32_t symbolSeqNum = static_cast<uint32_t>(message101.buffer[15] << 24) | static_cast<uint32_t>(message101.buffer[14] << 16) | static_cast<uint32_t>(message101.buffer[13] << 8) | message101.buffer[12];
            uint64_t orderID = (static_cast<uint64_t>(message101.buffer[23]) << 56) |(static_cast<uint64_t>(message101.buffer[22]) << 48) |(static_cast<uint64_t>(message101.buffer[21]) << 40) |(static_cast<uint64_t>(message101.buffer[20]) << 32) |static_cast<uint64_t>(message101.buffer[19] << 24) | static_cast<uint64_t>(message101.buffer[18] << 16) | static_cast<uint64_t>(message101.buffer[17] << 8) | message101.buffer[16];
            uint32_t price = static_cast<uint32_t>(message101.buffer[27] << 24) | static_cast<uint32_t>(message101.buffer[26] << 16) | static_cast<uint32_t>(message101.buffer[25] << 8) | message101.buffer[24];
            uint32_t volume = static_cast<uint32_t>(message101.buffer[31] << 24) | static_cast<uint32_t>(message101.buffer[30] << 16) | static_cast<uint32_t>(message101.buffer[29] << 8) | message101.buffer[28];
            uint8_t positionChange = static_cast<uint8_t>(message101.buffer[32]);
            char side;
            if(message101.buffer[33]==0x53) side = 'S';
            if(message101.buffer[33]==0x42) side = 'B';
            content << "," << sourceTimeNS << "," << symbolIDX << "," << symbolSeqNum << "," << orderID << "," << price << "," << volume << "," << side << ",," <<positionChange;
        }
        // deleteOrderMessage
        void parse102(stringstream& content, stringstream& prl_content, stringstream& trd_content, const message& message102){
            uint32_t sourceTimeNS = static_cast<uint32_t>(message102.buffer[7] << 24) | static_cast<uint32_t>(message102.buffer[6] << 16) | static_cast<uint32_t>(message102.buffer[5] << 8) | message102.buffer[4];
            uint32_t symbolIDX = static_cast<uint32_t>(message102.buffer[11] << 24) | static_cast<uint32_t>(message102.buffer[10] << 16) | static_cast<uint32_t>(message102.buffer[9] << 8) | message102.buffer[8];
            uint32_t symbolSeqNum = static_cast<uint32_t>(message102.buffer[15] << 24) | static_cast<uint32_t>(message102.buffer[14] << 16) | static_cast<uint32_t>(message102.buffer[13] << 8) | message102.buffer[12];
            uint64_t orderID = (static_cast<uint64_t>(message102.buffer[23]) << 56) |(static_cast<uint64_t>(message102.buffer[22]) << 48) |(static_cast<uint64_t>(message102.buffer[21]) << 40) |(static_cast<uint64_t>(message102.buffer[20]) << 32) |static_cast<uint64_t>(message102.buffer[19] << 24) | static_cast<uint64_t>(message102.buffer[18] << 16) | static_cast<uint64_t>(message102.buffer[17] << 8) | message102.buffer[16];
            content << "," << sourceTimeNS << "," << symbolIDX << "," << symbolSeqNum << "," << orderID <<",,,,,,,";
        }
        // orderExecutionMessage
        void parse103(stringstream& content, stringstream& prl_content, stringstream& trd_content, const message& message103){
            uint32_t sourceTimeNS = static_cast<uint32_t>(message103.buffer[7] << 24) | static_cast<uint32_t>(message103.buffer[6] << 16) | static_cast<uint32_t>(message103.buffer[5] << 8) | message103.buffer[4];
            uint32_t symbolIDX = static_cast<uint32_t>(message103.buffer[11] << 24) | static_cast<uint32_t>(message103.buffer[10] << 16) | static_cast<uint32_t>(message103.buffer[9] << 8) | message103.buffer[8];
            uint32_t symbolSeqNum = static_cast<uint32_t>(message103.buffer[15] << 24) | static_cast<uint32_t>(message103.buffer[14] << 16) | static_cast<uint32_t>(message103.buffer[13] << 8) | message103.buffer[12];
            uint64_t orderID = (static_cast<uint64_t>(message103.buffer[23]) << 56) |(static_cast<uint64_t>(message103.buffer[22]) << 48) |(static_cast<uint64_t>(message103.buffer[21]) << 40) |(static_cast<uint64_t>(message103.buffer[20]) << 32) |static_cast<uint64_t>(message103.buffer[19] << 24) | static_cast<uint64_t>(message103.buffer[18] << 16) | static_cast<uint64_t>(message103.buffer[17] << 8) | message103.buffer[16];
            uint32_t tradeID = static_cast<uint32_t>(message103.buffer[27] << 24) | static_cast<uint32_t>(message103.buffer[26] << 16) | static_cast<uint32_t>(message103.buffer[25] << 8) | message103.buffer[24];
            uint32_t price = static_cast<uint32_t>(message103.buffer[31] << 24) | static_cast<uint32_t>(message103.buffer[30] << 16) | static_cast<uint32_t>(message103.buffer[29] << 8) | message103.buffer[28];
            uint32_t volume = static_cast<uint32_t>(message103.buffer[35] << 24) | static_cast<uint32_t>(message103.buffer[34] << 16) | static_cast<uint32_t>(message103.buffer[33] << 8) | message103.buffer[32];
            uint8_t printFlag = static_cast<uint8_t>(message103.buffer[36]);
            char trd1 = static_cast<char>(message103.buffer[38]);
            char trd2 = static_cast<char>(message103.buffer[39]);
            char trd3 = static_cast<char>(message103.buffer[40]);
            char trd4 = static_cast<char>(message103.buffer[41]);
            
            //Look into only displaying when the ascii values are not blank or null
            content << "," << sourceTimeNS << "," << symbolIDX << "," << symbolSeqNum << "," << orderID << "," << price << "," << volume << ",,,," << printFlag << "," <<trd1 <<"," <<trd2 <<"," <<trd3<<"," <<trd4;
        }
        // replaceOrderMessage
        void parse104(stringstream& content, stringstream& prl_content, stringstream& trd_content, const message& message104){
            uint32_t sourceTimeNS = static_cast<uint32_t>(message104.buffer[7] << 24) | static_cast<uint32_t>(message104.buffer[6] << 16) | static_cast<uint32_t>(message104.buffer[5] << 8) | message104.buffer[4];
            uint32_t symbolIDX = static_cast<uint32_t>(message104.buffer[11] << 24) | static_cast<uint32_t>(message104.buffer[10] << 16) | static_cast<uint32_t>(message104.buffer[9] << 8) | message104.buffer[8];
            uint32_t symbolSeqNum = static_cast<uint32_t>(message104.buffer[15] << 24) | static_cast<uint32_t>(message104.buffer[14] << 16) | static_cast<uint32_t>(message104.buffer[13] << 8) | message104.buffer[12];
            uint64_t orderID = (static_cast<uint64_t>(message104.buffer[23]) << 56) |(static_cast<uint64_t>(message104.buffer[22]) << 48) |(static_cast<uint64_t>(message104.buffer[21]) << 40) |(static_cast<uint64_t>(message104.buffer[20]) << 32) |static_cast<uint64_t>(message104.buffer[19] << 24) | static_cast<uint64_t>(message104.buffer[18] << 16) | static_cast<uint64_t>(message104.buffer[17] << 8) | message104.buffer[16];
            uint64_t newOrderID = (static_cast<uint64_t>(message104.buffer[31]) << 56) |(static_cast<uint64_t>(message104.buffer[30]) << 48) |(static_cast<uint64_t>(message104.buffer[29]) << 40) |(static_cast<uint64_t>(message104.buffer[28]) << 32) |static_cast<uint64_t>(message104.buffer[27] << 24) | static_cast<uint64_t>(message104.buffer[26] << 16) | static_cast<uint64_t>(message104.buffer[25] << 8) | message104.buffer[24];
            uint32_t volume = static_cast<uint32_t>(message104.buffer[39] << 24) | static_cast<uint32_t>(message104.buffer[38] << 16) | static_cast<uint32_t>(message104.buffer[37] << 8) | message104.buffer[36];
            uint32_t price = static_cast<uint32_t>(message104.buffer[35] << 24) | static_cast<uint32_t>(message104.buffer[34] << 16) | static_cast<uint32_t>(message104.buffer[33] << 8) | message104.buffer[32];
            char side = static_cast<char>(message104.buffer[40]);
            //Look into only displaying when the ascii values are not blank or null
            content << "," << sourceTimeNS << "," << symbolIDX << "," << symbolSeqNum << "," << orderID << "," << price << "," << volume << "," << side << ",,,,,,,," <<newOrderID;
        }
        // addOrderRefreshMessage
        void parse106(stringstream& content, stringstream& prl_content, stringstream& trd_content, const message& message100){
            uint32_t sourceTime = static_cast<uint32_t>(message100.buffer[7] << 24) | static_cast<uint32_t>(message100.buffer[6] << 16) | static_cast<uint32_t>(message100.buffer[5] << 8) | message100.buffer[4];
            uint32_t sourceTimeNS = static_cast<uint32_t>(message100.buffer[11] << 24) | static_cast<uint32_t>(message100.buffer[10] << 16) | static_cast<uint32_t>(message100.buffer[9] << 8) | message100.buffer[8];
            uint32_t symbolIDX = static_cast<uint32_t>(message100.buffer[15] << 24) | static_cast<uint32_t>(message100.buffer[14] << 16) | static_cast<uint32_t>(message100.buffer[13] << 8) | message100.buffer[12];
            uint32_t symbolSeqNum = static_cast<uint32_t>(message100.buffer[19] << 24) | static_cast<uint32_t>(message100.buffer[18] << 16) | static_cast<uint32_t>(message100.buffer[17] << 8) | message100.buffer[16];
            uint64_t orderID = (static_cast<uint64_t>(message100.buffer[27]) << 56) |(static_cast<uint64_t>(message100.buffer[26]) << 48) |(static_cast<uint64_t>(message100.buffer[25]) << 40) |(static_cast<uint64_t>(message100.buffer[24]) << 32) |static_cast<uint64_t>(message100.buffer[23] << 24) | static_cast<uint64_t>(message100.buffer[22] << 16) | static_cast<uint64_t>(message100.buffer[21] << 8) | message100.buffer[20];
            uint32_t price = static_cast<uint32_t>(message100.buffer[31] << 24) | static_cast<uint32_t>(message100.buffer[30] << 16) | static_cast<uint32_t>(message100.buffer[39] << 8) | message100.buffer[28];
            uint32_t volume = static_cast<uint32_t>(message100.buffer[35] << 24) | static_cast<uint32_t>(message100.buffer[34] << 16) | static_cast<uint32_t>(message100.buffer[33] << 8) | message100.buffer[32];
            char side = static_cast<char>(message100.buffer[36]);

            //Look into only displaying when the ascii values are not blank or null
            string firm = "";
            firm+=static_cast<char>(message100.buffer[41])+static_cast<char>(message100.buffer[40])+static_cast<char>(message100.buffer[39])+static_cast<char>(message100.buffer[38])+static_cast<char>(message100.buffer[37]);
            content << sourceTime << "," << sourceTimeNS << "," << symbolIDX << "," << symbolSeqNum << "," << orderID << "," << price << "," << volume << "," << side << "," << firm;
        }
        // nonDisplayedTradeMessage
        void parse110(stringstream& content, stringstream& prl_content, stringstream& trd_content, const message& message110){
            uint32_t sourceTimeNS = static_cast<uint32_t>(message110.buffer[7] << 24) | static_cast<uint32_t>(message110.buffer[6] << 16) | static_cast<uint32_t>(message110.buffer[5] << 8) | message110.buffer[4];
            uint32_t symbolIDX = static_cast<uint32_t>(message110.buffer[11] << 24) | static_cast<uint32_t>(message110.buffer[10] << 16) | static_cast<uint32_t>(message110.buffer[9] << 8) | message110.buffer[8];
            uint32_t symbolSeqNum = static_cast<uint32_t>(message110.buffer[15] << 24) | static_cast<uint32_t>(message110.buffer[14] << 16) | static_cast<uint32_t>(message110.buffer[13] << 8) | message110.buffer[12];
            uint32_t tradeID = static_cast<uint32_t>(message110.buffer[19] << 24) | static_cast<uint32_t>(message110.buffer[18] << 16) | static_cast<uint32_t>(message110.buffer[17] << 8) | message110.buffer[16];
            uint32_t price = static_cast<uint32_t>(message110.buffer[23] << 24) | static_cast<uint32_t>(message110.buffer[22] << 16) | static_cast<uint32_t>(message110.buffer[21] << 8) | message110.buffer[20];
            uint32_t volume = static_cast<uint32_t>(message110.buffer[27] << 24) | static_cast<uint32_t>(message110.buffer[26] << 16) | static_cast<uint32_t>(message110.buffer[25] << 8) | message110.buffer[24];
            uint8_t printFlag = static_cast<uint8_t>(message110.buffer[28]);
            char trd1 = static_cast<char>(message110.buffer[29]);
            char trd2 = static_cast<char>(message110.buffer[30]);
            char trd3 = static_cast<char>(message110.buffer[31]);
            char trd4 = static_cast<char>(message110.buffer[32]);
            
            //Look into only displaying when the ascii values are not blank or null
            content << "," << sourceTimeNS << "," << symbolIDX << "," << symbolSeqNum << ",," << price << "," << volume << ",,,," << printFlag << "," <<trd1 <<"," <<trd2 <<"," <<trd3<<"," <<trd4<<","<<tradeID;
        }

    public:
        nyse_parser(int batch, string inputFile, string outputFile, string prlFile, string trdFile){
            batch_size = batch;
            input_file = inputFile;
            output_file = outputFile;

            // Intialize PRL and TRD file paths
            prl_output_file = prlFile;
            trd_output_file = trdFile;

            // Open and store the bytes into a vector:
            ifstream input(input_file,ios::binary);
            if(!input.is_open()){
                cout << "Could not open file " << input_file << "."<< endl;
                // Add exception handling
            }

            char byte;
            input.seekg(0, ios::end);
            streamsize fileSize = input.tellg();
            input.seekg(0, ios::beg);
            bytes.reserve(fileSize);
            vector<char> buffer(2048);
            while (input.read(buffer.data(), buffer.size())) {
                bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + input.gcount());
            }
            bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + input.gcount());
            cout << bytes.size() << endl;

            // Skip the global header-24 bytes:
            bytes.erase(bytes.begin(),bytes.begin()+24);

            // Copy bytes to array:
            unsigned char* buf = new unsigned char[bytes.size()];
            bufPointer = buf;
            origPointer = buf;
            copy(bytes.begin(),bytes.end(),buf);
        }

        ~nyse_parser() {
            delete origPointer;
        }

        void parse() {
            ofstream output(output_file);
            ofstream prl_output(prl_output_file);
            ofstream trd_output(trd_output_file);

            if(!output.is_open()){ 
                cout << "Could not open file " << output_file <<"." << endl;
            }
            if(!prl_output.is_open()){
                cout << "Could not open file " << prl_output_file <<"." << endl;
            }
            if(!trd_output.is_open()){
                cout << "Could not open file " << trd_output_file <<"." << endl;
            }

            // Write CSV Headers
            output << "PacketCaptureTime,SendTime,SendTimeNS,MsgSize,MsgType,SourceTime,SourceTimeNS,SymbolIndex,SymbolSeqNum,OrderID,Price,Volume,Side,Firm,PositionChange,PrintFlag,Trd1,Trd2,Trd3,Trd4,TradeID\n";
            prl_output << "PacketCaptureTime,SendTime,Price,Side,Symbol,Size,RecordType,OrderID,NumParticipants,EventFlag,TickType\n";
            /* PRL SCHEMA BREAKDOWN
             * PacketCaptureTime -- use arrival timestamp
             * SendTime -- timestamp when NYSE message was sent
             * MessageID -- unique id for message
             * RawTimestamp -- full timestamp including nanoseconds for precision
             * TickType -- derived BID or ASK, determine with side (S -> ASK)
             * Symbol -- mapped with SymbolIDX, mapped readable ticker symbol
             * Price -- price level from message
             * Size -- volume at this price level
             * RecordType -- msgtype -- indicates type of data 100, 101, 103
             * Flag -- flag -- indicated special conditions or message flags
             * Ask -- true for ASK, false for BID
             */
            trd_output << "PacketCaptureTime,SendTime,MessageID,RawTimestamp,TickType,Symbol,Price,Size,RecordType,Flag,Ask\n";

            int byteNum = bytes.size();
            int packetNum = 0;
            int batchNum = 0;
            messageList.reserve(1000);
            packHeader temp;

            while(byteNum > 0){
                // get packet length to avoid having to check for VLAN trailer:
                uint32_t pcapLength = (static_cast<uint32_t>((static_cast<uint16_t>(bufPointer[11])<<8) | bufPointer[10])<<16) | ((static_cast<uint16_t>(bufPointer[9])<<8) | bufPointer[8]);
                uint64_t packetCaptureTimeTotal = (static_cast<uint32_t>((static_cast<uint16_t>(bufPointer[3])<<8) | bufPointer[2])<<16) | ((static_cast<uint16_t>(bufPointer[1])<<8) | bufPointer[0]) + (static_cast<uint32_t>((static_cast<uint16_t>(bufPointer[7])<<8) | bufPointer[6])<<16) | ((static_cast<uint16_t>(bufPointer[5])<<8) | bufPointer[4]);

                // skip pcap packet header - 16 bytes
                bufPointer+=16;
                byteNum-=16;

                // skip networks headers-46 bytes
                bufPointer+=46;
                byteNum-=46;
                
                // NYSE packet header-16 bytes
                temp.packetSize = static_cast<uint16_t>(bufPointer[1] << 8) | bufPointer[0];
                temp.sendTime = static_cast<uint32_t>(bufPointer[11] << 24) | static_cast<uint32_t>(bufPointer[10] << 16) | static_cast<uint32_t>(bufPointer[9] << 8) | bufPointer[8];
                temp.sendTimeNS = static_cast<uint32_t>(bufPointer[15] << 24) | static_cast<uint32_t>(bufPointer[14] << 16) | static_cast<uint32_t>(bufPointer[13] << 8) | bufPointer[12];

                // parse each specific message type:
                int count = 0;
                message tempMessage;
                unsigned char* tempPointer = bufPointer;
                tempPointer += 16;

                while(count < (temp.packetSize - 16)){
                    tempMessage.packetCaptureTime = packetCaptureTimeTotal;
                    tempMessage.sendTime = temp.sendTime;
                    tempMessage.sendTimeNS = temp.sendTimeNS;
                    tempMessage.buffer = tempPointer;
                    tempMessage.msgSize = static_cast<uint16_t>(tempPointer[1] << 8) | tempPointer[0];
                    tempMessage.msgType = static_cast<uint16_t>(tempPointer[3] << 8) | tempPointer[2];
                    count += tempMessage.msgSize;
                    tempPointer += tempMessage.msgSize;
                    messageList.push_back(tempMessage);
                }

                // write to csv
                if(messageList.size() > batch_size){
                    cout << "batch " << batchNum << " completed" << endl;
                    writeToFile(messageList, output, prl_output, trd_output);
                    messageList.clear();
                    batchNum++;
                }

                // proceed to next package:
                bufPointer += pcapLength;
                bufPointer -= 46;
                byteNum -= pcapLength;
                byteNum += 46;
                packetNum++;
            }

            if(!messageList.empty()){
                writeToFile(messageList, output, prl_output, trd_output);
            }

            cout << packetNum << endl;
            output.close();
            prl_output.close();
            trd_output.close();
        }

        void writeToFile(const vector<message>& messList, ofstream& outputFile, ofstream& prlOutput, ofstream& trdOutput){
            stringstream content, prlContent, trdContent;

            for(const message& tempM: messList) {
                content << tempM.packetCaptureTime << "," << tempM.sendTime << "," << tempM.sendTimeNS << "," << tempM.msgSize << "," << tempM.msgType << ",";
                if(tempM.msgType==100) parse100(content,prlContent,trdContent,tempM);
                if(tempM.msgType==101) parse101(content,prlContent,trdContent,tempM);
                if(tempM.msgType==102) parse102(content,prlContent,trdContent,tempM);
                if(tempM.msgType==103) parse103(content,prlContent,trdContent,tempM);
                if(tempM.msgType==104) parse104(content,prlContent,trdContent,tempM);
                if(tempM.msgType==106) parse106(content,prlContent,trdContent,tempM);
                if(tempM.msgType==110) parse110(content,prlContent,trdContent,tempM);
                content << "\n";
            }
            outputFile << content.str();
            prlOutput << prlContent.str();
            trdOutput << trdContent.str();
        }
};

// used for testing
// int main() {
//     string in = "NYSE_example.pcap";
//     string out = "orderbookCombo.csv";
//     string out_prl = "NYSE_PRL_output.csv";
//     string out_trd = "NYSE_TRD_output.csv";

//     auto start = chrono::high_resolution_clock::now();
//     nyse_parser pcap(100000,in,out,out_prl,out_trd);
//     pcap.parse();
//     auto end = chrono::high_resolution_clock::now();
//     std::chrono::duration<double> struct_duration = end - start;
//     std::cout << "Time: " << struct_duration.count() << " seconds\n";

//     return 0;
// }
