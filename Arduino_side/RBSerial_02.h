/*
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
*/


#ifndef RBSERIAL_02_H
#define RBSERIAL_02_H

#include <Arduino.h>
#include "KB_Aux.h" // for KB_dtoa()

//#define DEBUG_MODE
#define DOUBLE_AKN    // Send AKN twice

#define RBS_SER_TIMEOUT    10           // ms, used in receiveFrame() to wait for serial data, calculated according to bour rate. for 115200 boud rate default is 20
                                        // (21 byte frame = 168bits, boud rate 115200bits/s, 168/115200 ~= 1.5ms, enough)
#define RBS_RESP_WAIT_TIME 100         // ms, used in receiveARQ() AND transmitARQ(), max time to wait for AKN responce, default 100
#define RBS_ARQ_ATT_MAX     5           // Used in transmitARQ(), number of attempts to send Frame to receiving aknowlegment
#define RBS_COMM_TX_ATT_MAX    100      // Used in commSendParsedFrame() and commSendCommand(), how many times to call transmitARQ() before reset communication if not success
#define RBS_COMM_RX_ATT_MAX    500      // Used in commReceive(), how many times to call receiveARQ() before reset communication if not success

#define RBS_ARG_LMAX       16           // max number of bytes for received value 
#define RBS_FRM_SIZEMIN     5           // min size of Frame
#define RBS_FRM_FST       '['           // 0x5B "[" Opening bracket 
#define RBS_FRM_LST       ']'           // 0x5D "]" Closing bracket
#define RBS_TYPE_GET      '?'           // 0x3F "?" request for value
#define RBS_TYPE_SET      '='           // 0x3D "=" set value
#define RBS_TYPE_AT       '@'           // 0x3D "=" set value
#define RBS_LBL_HDS       '#'           // Handshake message message
#define RBS_SN_ZERO       '0'           // Frame serial number '0'
#define RBS_SN_ONE        '1'           // Frame serial number '1'
#define RBS_ARG_AKN       '+'           // argument "AKN" for Hendshake labeled message
#define RBS_ARG_NAK       '-'           // argument "NAK" for Hendshake labeled message
#define RBS_ARG_RDY       '>'           // argument "RDY" for Hendshake labeled message
#define RBS_ARG_RST       '!'           // argument "RST" for Hendshake labeled message

#define RBS_DBL_PREC       4            // precision for double type when conwert to string

// Parser state machine states
enum class RBSParserState{
  FRM_NOT_RECEIVED,
  FRM_NOT_STARTED,
  FRM_RECEIVED,
  FRM_STARTED,
  SN_RECEIVED,
  LBL_RECEIVED,
  TYPE_AT_RECEIVED,
  TYPE_GET_RECEIVED,
  TYPE_SET_RECEIVED,
  ARG_BYTE_RECEIVED
};


// Receiver state machine states
enum class RBSReceiverState{
  RX_ACCEPT_FRAME,      
  RX_END,               
  RX_DISCARD_FRAME,   
  RX_RST_RECEIVED,  
  RX_SEND_AKN_0,        
  RX_SEND_AKN_1,        
  RX_START,             
  RX_WAIT_FRAME_0,      
  RX_WAIT_FRAME_1       
};


// Transmitter state machine states
enum class RBSTransmitterState{
  TX_CONFIRMED,         
  TX_FAIL, 
  TX_RST_RECEIVED,             
  TX_SEND_FRAME_0,      
  TX_SEND_FRAME_1,      
  TX_START,             
  TX_WAIT_AKN_0,        
  TX_WAIT_AKN_1
};



class RBSerial
{
  private:
   
    HardwareSerial* rbs_serial;   // ptr to HardwareSerial object

    // Parser related
    RBSParserState parser_state;  // Parser FSM state    
    bool frameReceived;           // Frame received flag
    char incomingByte;            // last received byte
    char parsedSn;                // parsed sequence number
    char parsedLbl;               // parsed command label
    char parsedType;              // parsed command type
    char* parsedArgArr;           // parsed command argument
    char* Frame;                  // pointer to Frame (every time allocated/deallocated before/after sending)
    int currArgIdx;               // counter/index of element in parsedArgArr to save incoming byte

    // Predefined frames (to be initialised in constructor)
    char* predefFrameAkn0;        // Predefined frame with AKN0 message (will be initialised in constructor)
    char* predefFrameAkn1;        // Predefined frame with AKN1 message (will be initialised in constructor)
    char* predefFrameNak;         // Predefined frame with NAK message (will be initialised in constructor)
    char* predefFrameRdy;         // Predefined frame with RDY message (will be initialised in constructor)
    char* predefFrameRst;         // Predefined frame with RST message (will be initialised in constructor)

    // Receiver related
    RBSReceiverState rx_state;    // Receiver FSM state
    bool acceptFrame;             // is received frame accepted
    char expectedRxFrameSn;       // expected sequence number of received frame
    int rxNotReceivedCnt;

    // Transmitter related
    bool txConfirmed;             // is transmission confirmed
    RBSTransmitterState tx_state; // Transmitter FSM state
    char txFrameSn;               // Sequence number of a Frame to transmit
    int txAttCnt;                 // counter for transmission attempts
    
    unsigned long waitTmr;     // Timer to measure waiting for akn time

  public:

    // Constructor and destructor
    RBSerial();
    ~RBSerial();

    // Adding HardwareSerial object
    void addSerial(HardwareSerial* serialPtr);

    // Getters / Setters
    bool getIsFrameReceived();
    void setIsFrameReceived(bool isReceived);

    char getParsedSn();
    void setParsedSn(char frmSn);

    char getParsedLbl();
    void setParsedLbl(char cmdLbl);

    char getParsedType();
    void setParsedType(char cmdType);

    char* getParsedArgArrPtr();
    void setParsedArgArr(char* arr, int elCnt);
    void setParsedArgArr(char argChar);
  
    // Core
    bool receiveFrame();            // receive Frame via serial (byte by byte), parse it using FSM, save data into correspinding "parsedXXX" members
    void sendParsedFrame();         // allocate and create Frame from data stored in "parsedXXX" members, send it via serial, deallocate frame
    void sendFrame(char* arr);      // send char array via serial (char array supposed to be correctly formed Frame)

    // "Basic" ARQ
    bool receiveARQ();              // receive Frame according to Stop and Wait ARQ protocol: send AKN(1) after Frame(0) received, send AKN(0) after Frame(1) received and so on
    bool transmitARQ();             // send Frame according to Stop and Wait ARQ protocol: send Frame(0) and wait for AKN(1), send Frame(1) and wait for AKN(0) and so on
    void transmitRdy();

    // "HIgh level" / "Transactions"
    bool commInquiryData(char dataLbl);                             // For master: Send Frame with inquiry and receive Frame with Answer, ARQ Stop&Wait + reset RX/TX on error
    
    // For master: Send Frame with set-type command, ARQ Stop&Wait + reset both parties RX/TX on error
    bool commSendCommand(char dataLbl, int dataVal);                // Overload for int
    bool commSendCommand(char dataLbl, double dataVal);             // Overload for double
    bool commSendCommand(char dataLbl, unsigned long dataVal);      // Overload for unsigned long
    bool commSendCommand(char dataLbl, char dataVal);               // Overload for char
    bool commSendCommand(char dataLbl, char* dataVal, int elCnt);   // Overload for char array of elCnt elements    
    bool commSendParsedFrame();                                     // For master: Send parsed frame, ARQ Stop&Wait + reset both parties RX/TX on error   
    
    bool commReceive();                                             // For master/slave: Receive frame, reset both parties RX/TX if long time no incoming frames 

    // Char array input/retreaving
    void intToArgArr(int val);                  // convert int to char aarray "parsedArgArr" 
    void dblToArgArr(double val);               // convert double to char array "parsedArgArr"
    void ulongToArgArr(unsigned long val);      // convert unsigned long to char array "parsedArgArr"
    int argArrToInt();                          // convert char array "parsedArgArr" to int
    double argArrToDbl();                       // convert char array "parsedArgArr" to double
    unsigned long argArrToULong();              // convert char array "parsedArgArr" to unsigned long

    // Check parsed data
    bool checkLbl(char testLbl);                // return true if testLbl == parsedLbl
    bool checkSn(char testSn);                  // return true if testLbl == parsedSn
    bool checkType(char testType);              // return true if testLbl == parsedType
    bool checkArgChar(char testArgChar);        // return true if single char testLbl == parsedArgArr
    bool checkArgString(String testArgString);  // return true if string testLbl == parsedArgArr
    bool checkRST();                            // return true if "reset" handshake message received
    

    // Aux
    void resetRX();                 // Reset receiver
    void resetTX();                 // Reset transmitter
    void sendRST();                 // Send heandshake message to partner with command to resent its RX and TX
    void resetComm();               // reset RX and TX on both sides
    
    void clearArgArr();                         // set all chars = '/0' in the "parsedArgArr" array
    void resetParser();                         // set all "parsedXXX" members = '/0', clear frameReceived flag, reset currArgIdx counter
    
    void allocFrame();                          // allocate char array and create a Frame from data stored in "parsedXXX" members
    void updateFrameSn(char frmSn);             // update sequence number ('0' or '1') of the allocated Frame
    void deallocFrame();                        // deallocate char array with Frame
 
    void clearSerialBuffer();                   // clear serial buffer


    // display members
    void disp();
};

#endif
