# Serial-ARQ-protocol-for-Arduino

Serial ARQ protocol for Arduino
Version 2.1 2026-10-08
 - transmitRdy() added
   This function is to send READY confirmation:
   It means that receiver in not busy and ready to receive new data or command
   (Not the same as RBS ARQ protocol AKN.
   AKN only confirms receiving of frame, but doesnt confirm that receiver is ready)


Version 2.0 2026-05-11 finished 2026-05-26
 - Stop and Wait ARQ protocol

1 FRAME
-------
Frame format:
'[' 's/n' 'lbl' 'type' ['arg'] ']'
Frame fields:
'[' - frame start symbol, char, 1 byte
's/n' -   frame sequence number, {'0', '1'}, char, 1 byte. Will be set automatically for each frame according to protocol-run logic;
'lbl' -   label/tag for command or data, {any} ('#' reserved for "handshake" commands), char, 1 byte;
'type' -  type of a message, {'?', '=', '@'}, char, 1 byte; 
          '?' - "Get" message, used to inquire some data value from receiver;
          '=' - "Set" message, used to set some data value at receiver;
          '@' - "At" message, used to report some data value;
['arg'] -   message or some data/value, char array, 0 bytes for type-"get" messages, 1-16 bytes for "ser" and "at" messages;
          '+' - reserved for "handshake" AKN message;
          '-' - reserved for "handshake" NAK message;
          '>' - reserved for "handshake" RDY message ("ready");
          '!' - reserved for "handshake" RST message ("reset");
']' - frame end symbol, char, 1 byte

Frame examples:
 - "[0T?]" Inquity temperature from a sensor, frame s/n = '0' (char[5] Frame = {'[','0','T','?',']'})
 - "[1T@21.3]" Report temperature from a sensor, frame s/n = '1'  (char[9] Frame = {'[','1','T','@','2','1','.','3',']'})
 - "[0P=75]" Set pwm for a motor, frame s/n = '0'  (char[7] Frame = {'[','0','P','=','7','5',']'})


2 ARQ STOP AND WAIT (PING PONG)
-------------------------------
pre-defined "akn", "rst"... etc handshake messages will be sent automatically according to protocol-run logic;

TX: Send data frame s/n = '0' and waits for akn message with s/n = '1'.
      if akn with s/n = '1' received - transmission confirmed, next frame to be sent has s/n = 1;
      if akn with s/n = '1' not received - transmission not confirmed, repeat transmission;
    Send data frame s/n = '1' and waits for akn frame with s/n = '0'.
      if akn with s/n = '0' received - transmission confirmed, next frame to be sent has s/n = 0;
      if akn with s/n = '0' not received - transmission not confirmed, repeat transmission;

RX: Wait frame with s/n = '0'.
      if frame with s/n = '0' received - accept frame, send akn message with s/n = '1', next frame to wait has s/n = '1';
      if frame with s/n = '0' not received - discard frame, continue to wait
    Wait frame with s/n = '1'.
      if frame with s/n = '1' received - accept frame, send akn message with s/n = '0', next frame to wait has s/n = '0';
      if frame with s/n = '1' not received - discard frame, continue to wait


3 ERROR HANDLING
----------------
If no correct handhske messages or no frames with expected s/n received for a long time (or after many attempts) - "rst" handshake message will be sent automatically
and communication protocol sequense numbers will be reset

Below "high"-level methods implements this type error handling
as well as "akn" and sequence number checks
    bool commReceive();                       // For master/slave: Receive frame, reset both parties RX/TX if long time no incoming or series of errors 
    bool commInquiryData(char dataLbl);       // For master: Send Frame with inquiry and receive Frame with Answer, ARQ Stop&Wait + reset RX/TX on error
    bool commSendCommand(char dataLbl, int/double/unsigned long dataVal);     // For master: Send Frame with set-type command, ARQ Stop&Wait + reset both parties RX/TX on error  
    bool commSendCommand(char dataLbl, char* dataVal, int elCnt);             // Overloaded for char array of elCnt elements

"Basic" ARQ methods DOES NOT implement this this type error handling
They only implements "akn" and sequence number checks
    bool receiveARQ();              // receive Frame according to Stop and Wait ARQ protocol: send AKN(1) after Frame(0) received, send AKN(0) after Frame(1) received and so on
    bool transmitARQ();             // send Frame according to Stop and Wait ARQ protocol: send Frame(0) and wait for AKN(1), send Frame(1) and wait for AKN(0) and so on


4 INCOMING/OUTCOMING SATA
-------------------------
4.1 Incoming data
Incoming frame can be received by polling using "high"-level method
    bool commReceive();           // For master/slave: Receive frame, reset both parties RX/TX if long time no incoming or series of errors 
Or by "Basic" ARQ method
    bool receiveARQ();    
Incoming frame will be parsed into these fields/member variables:
    char parsedSn;                // parsed sequence number 
    char parsedLbl;               // parsed data/command label
    char parsedType;              // parsed message type
    char* parsedArgArr;           // parsed data value/argument
parsed values can be checked using these methods
    bool checkLbl(char testLbl);                // return true if testLbl == parsedLbl
    bool checkSn(char testSn);                  // return true if testLbl == parsedSn
    bool checkType(char testType);              // return true if testLbl == parsedType
    bool checkArgChar(char testArgChar);        // return true if single char testLbl == parsedArgArr
    bool checkArgString(String testArgString);  // return true if string testLbl == parsedArgArr
or they can be accessed using getters:
    char getParsedSn();
    char getParsedLbl();
    char getParsedType();
    char* getParsedArgArrPtr();
numeric values from parsedArgArr char array can be retrieved using these methods
    int argArrToInt();                          // convert char array "parsedArgArr" to int
    double argArrToDbl();                       // convert char array "parsedArgArr" to double
    unsigned long argArrToULong();              // convert char array "parsedArgArr" to unsigned long

4.2 Outcoming data
Outcoming data frame will be constructed from same fields/member variables:
    char parsedSn;                // parsed sequence number (it will be updated automatically according to protocol-tun logic)
    char parsedLbl;               // parsed data/command label
    char parsedType;              // parsed message type
    char* parsedArgArr;           // parsed data value/argument
Data frame can be sent using "high"-level methods (frame fields will be updated according to arguments passed)
    bool commInquiryData(char dataLbl);  // For master: Send Frame with inquiry and receive Frame with Answer, ARQ Stop&Wait + reset RX/TX on error
    bool commSendCommand(char dataLbl, int/double/unsigned long dataVal);  // For master: Send Frame with set-type command, ARQ Stop&Wait + reset both parties RX/TX on error  
    bool commSendCommand(char dataLbl, char* dataVal, int elCnt);   // Overloaded for char array of elCnt elements
Or data frame fields can be updated by setters first, and then frame will be constructed and sent using "Basic" ARQ method
    void setParsedLbl(char cmdLbl);
    void setParsedType(char cmdType);
    void setParsedArgArr(char* arr, int elCnt);
    void setParsedArgArr(char argChar);
numeric values can be loaded into parsedArgArr char arrey using these methods:
    void intToArgArr(int val);                  // convert int to char aarray "parsedArgArr" 
    void dblToArgArr(double val);               // convert double to char array "parsedArgArr"
    void ulongToArgArr(unsigned long val);      // convert unsigned long to char array "parsedArgArr"
"Basic" ARQ method to transmit    
    bool transmitARQ();             // send Frame according to Stop and Wait ARQ protocol: send Frame(0) and wait for AKN(1), send Frame(1) and wait for AKN(0) and so on

Refs:
[1] GeeksForGeeks, “Stop and Wait ARQ,” GeeksForGeeks. Accessed: May 24, 2026. [Online]. 
    Available: https://www.geeksforgeeks.org/computer-networks/stop-and-wait-arq/
[2] G. J. Holzmann, Design and validation of computer protocols. in Prentice Hall software series. Englewood Cliffs, N.J: Prentice Hall, 1991.
[3] “pass a reference(or pointer) of a Serial object to the function(),” Arduino Forum. Accessed: May 24, 2026. [Online]. 
    Available: https://forum.arduino.cc/t/pass-a-reference-or-pointer-of-a-serial-object-to-the-function/152276
