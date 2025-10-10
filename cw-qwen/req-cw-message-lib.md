I want to create a lib to generate and parse capwap messages.
This library implemented with C. and created under directory capwap-lib
For generating message: the caller will pass the pointer to structure of the message, and pass the pointer to buffer of UTF8, and the capacity of this buffer. The generate method will generate the message and write to the buffer, and return the length of the message generated. return negative error code if any errors. And please check the buffer capacity when write in case overflow.

For parsing the message: the caller will pass the pointer to the buffer and the length of the data. And pass the pointer to the structure of the message. The method will parse the message and write to the information to the structure. When parse the message, each field of the structure only pointer to the data, but don't clone or hold the data. The data will actually still keep in the buffer.

provide generate and parse for all capwap messages. 
also create unit test cases for each methods
provide scripts to build and test 
provide readme document.
