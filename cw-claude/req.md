# description
I want to create a capwap test suit to test capwap client application and capwap server application. The test suit should implemented with C. For the capwap protocol, can reference to rfc5415.

# basic requirement
## constant value
Please define meaningful macro for constant values, and avoid directly use constant value in the code

## event handle
create and use epoll to handle socket and timer events

## scripts
- provide scripts each module to build, package and test.
- provide scripts the test suit.

## other
- provide readme
- All modules which can run as application should be able to gracefully shutdown or forced shutdown. 
- All modules which can run as application should have configuration file.
- create sub folder for each module. the folder name is same as module name.
- provide function to convert error code from int to string, and log the error code with string format whenever call failed.


# modules
there are 3 modules, I introduce them later
- cw-log
- cw-msg-lib
- cw-dtls
- cw-client-tester
- cw-server-tester

## cw-log
- This module is a library used by other modules.
- This module implemented the log system for all modules.
- The log should be leveled, the log level are TRACE, DEBUG, INFO, WARN, ERROR. The log should be provide information log-level, time, source-file, line-in-source-file, function-name and detail information
- The log could be saved as pure text, or csv, or json format. The log file should be rollover based on size
- The log can be configured in configure file
- The log should be clear, simple, concise and provide enough information
- Should provide unit test cases for this module.

## cw-msg-lib
- This module is a library use by other modules.
- This library implemented with C. and created under directory capwap-lib
For generating message: the caller will pass the pointer to structure of the message, and pass the pointer to buffer of UTF8, and the capacity of this buffer. The generate method will generate the message and write to the buffer, and return the length of the message generated. return negative error code if any errors. And please check the buffer capacity when write in case overflow.
- For parsing the message: the caller will pass the pointer to the buffer and the length of the data. And pass the pointer to the structure of the message. The method will parse the message and write to the information to the structure. When parse the message, each field of the structure only pointer to the data, but don't clone or hold the data. The data will actually still keep in the buffer.
- provide generating and parsing functions for all capwap messages. 
- log error with error code as string whenever call failed
- create unit test cases for each methods


## cw-dtls
- This module implement the dtls layer of capwap. This module is a library. This module can use cw-log to write log. 
- The dtls use openssl lib implement encryption and decryption. The certification information can be initialized or passed by caller.


## cw-client-tester
- This module is an application to test CAPWAP client applications. 
- This application run as stonealone application and can test CAPWAP client based on startup option. 
- This application act as like a CAPWAP server.
- This application use cw-log for log, use cw-dtls for encryption and decryption; use cw-msg-lib for generating and parsing message;
- This application run test cases. 
- Each test case has a meaningful test case name, short description of the test case, and a workflow.
- The workflow has state and event driven. The state can use capwap protocal state. The workflow verify the event based on state, and then handle the event and take actions accordingly, and migrate state if required
For each workflow, plese log the summary at the end of the workflow. the summary should contain information like workflow execute result, spent time(ms). And log detail reason why it failed and context information if the workflow was actually failed.
- The application should also act as a capwap server, when receive the message, parsed message, update the state and context, also need to generate response message to the peer. Please reference to RFC5415 and lib capwap-lib for what messages should be generated and the message format.


## cw-server-tester
- This module is an application to test CAPWAP server applications. 
- This application run as stonealone application and can test CAPWAP server based on startup option. 
- This application act as like a group of CAPWAP clients.
- This application use cw-log for log, use cw-dtls for encryption and decryption; use cw-msg-lib for generating and parsing message;
- This application act as CAPWAP clients and able to discover the CAPWAP server, and only chose one CAPWAP server to test. It either chose the first CAPWAP server which responsed or chose the preferred one which user configured in config file.
- This application start a thread pool to manage mulitple threads, and each thread maintain one CAPWAP client. The capacity of the thread pool is configured in config file.
- One thread of the application can run multiple test cases sequentially. 



