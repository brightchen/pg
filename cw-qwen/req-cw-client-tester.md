I am developing an application to test CAPWAP client. This application run as stonealone application and can test CAPWAP client based on startup option. 
Please create directory cw-client-tester for this application. and put all resource related to this application into this directory.

This application act as a CAPWAP server and test CAPWAP client. The application will start and handle CAPWAP client discovery message, and reply with discovery response.
The application can be gracefully shutdown or forced shutdown. 
This application use the lib under capwap-lib to parse and generate messsages.
This application run test cases. 
Each test case has a meaningful test case name, short description of the test case, and a workflow.
The workflow has state and event driven. The state can use capwap protocal state. The workflow verify the event based on state, and then handle the event and take actions accordingly, and migrate state if required
For each workflow, plese log the summary at the end of the workflow. the summary should contain information like workflow execute result, spent time(ms). And log detail reason why it failed and context information if the workflow was actually failed.
The application should have a levled log system, the log level are TRACE, DEBUG, INFO, WARN, ERROR. The log should be provide information log-level, time, source-file, line-in-source-file, function-name and detail information
The log could be saved as pure text, or csv, or json format. The log file should be rollover based on size
The log can be configured in configure file

The application should also act as a capwap server, when receive the message, parsed message, update the state and context, also need to generate response message to the peer. Please reference to RFC5415 and lib capwap-lib for what messages should be generated and the message format.
