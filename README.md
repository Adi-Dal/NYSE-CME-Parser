# NYSE-CME-Parser
NYSE/CME parsers built in C++. Version 2 utilizes multithreading to parse various formats concurrently decrease parse times. NYSE Parsers follow Chicago(now Austin) specifications. CME specifications can be found on their website but need an account to get the schema files if you want to use the inbuilt functions to help parse like was shown in the code.

Overall Logic and Steps:
1) Research the schemas for the exchnage you will be writing the parser for.
2) Copy the bytes of the pcap into memory so you can begin parsing them.
3) The next step is to skip the pcap specific headers as they are found in every pcap and aren't all needed for parsing. Each pcap file will have global headers and packet header. You can skip the global header of 24 bytes as that won't affect the data you will be parsing.
4) Of the 16 bytes of packet headers for each message make sure to keep track of packet length to ensure you are parsing the packets correctly as relying on the exchange for accurate size information can be problematic due to the presence of variable network headers, incorrect values, etc.
5) Next traverse the array, keeping track of the packet lengths and header lengths, and begin segmenting the data into individual packets to parse based off of the message type(the details for which should be found in the exhange's schema).
6) As you traverse the array it helps to use techniques like parsing in batches at a time as that reduces the number of I/O operations needed overall.
7) To further improve runtime you can use multithreading to parse different message types to different files as was shown in the multithreadedNYSEparser. This approach only works when outputting to different files, however, as order is not guranteed when using threads.

Further Tips:
1) Open the packets in wireshark and try manually parsing some of them to make sure your output is correct if making a parser from scratch. Furthermore, Wirehark is also helpful when it comes to finding out the lengths of network headers and other header you might have to account for when writing parsers in general.
2) Get the schema from CME as they are maintained and can be extracted into helper functions using the sbe tool found here: https://github.com/aeron-io/simple-binary-encoding

Problems Encountered:
1) Incorrect parsing. This was due to reliance on the exchange's length field to determine the length of the packets which was oftentimes incorrect as there may be trailing garbage bytes at the end of the packet due to things like network header requirements. See VLAN comment in the NYSE parser for an exmaple of this.
2) When using the SBE tool use quotations around the flags and their values as it wouldn't work when I tried without the usage of quotes.



Link to full project this code was made for:
https://gitlab.engr.illinois.edu/ie421_high_frequency_trading_fall_2024/ie421_hft_fall_2024_group_02/group_02_project

Anton Charov's Github:
