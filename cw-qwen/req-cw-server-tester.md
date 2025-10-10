I am developing an application to test CAPWAP server. This application run as stonealone application and can test CAPWAP server. 
This application act as CAPWAP clients and able to discover the CAPWAP server, and only chose one CAPWAP server to test. It either chose the first CAPWAP server which responsed or chose the preferred one which user configured in config file.
It can start a thread pool to manage mulitple threads, and each thread maintain one CAPWAP client. The capacity of the thread pool is configured in config file.
The application or one thread of the application can run multiple test cases sequentially. 
The application be gracefully shutdown or forced shutdown. 
This application use cw-message-lib to parse and generate messsages.
Please define meaningful macro for constant value, and replace the constant value with th macro

Please verify the capwap_header_t, seems there are some error over there, such as Version should be 4 bits, type 4 bits. please use the format defined in RFC5415 

Please add the dtls layer. The dtls used encrypt and decrypt the message between this application and its peer.
