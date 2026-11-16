#include "RBSerial_02.h"


// Constructor
//----------------------------------------------------------------------
RBSerial::RBSerial(){
 
  frameReceived = false;
  rbs_serial = NULL;
  parser_state = RBSParserState::FRM_NOT_STARTED;
  incomingByte = '\0';
  parsedSn = '\0';
  parsedLbl = '\0';
  parsedType = '\0';
  parsedArgArr = new char[RBS_ARG_LMAX];
  RBSerial::clearArgArr();
  Frame = NULL;
  currArgIdx = 0;

  // Predefined AKN0 frame
  predefFrameAkn0 = new char[7];
  predefFrameAkn0[0] = RBS_FRM_FST;         // Frame start
  predefFrameAkn0[1] = RBS_SN_ZERO;         // Frame S/N = '0'
  predefFrameAkn0[2] = RBS_LBL_HDS;         // LBL = "Heandshake message"
  predefFrameAkn0[3] = RBS_TYPE_AT;         // TYPE = "at"
  predefFrameAkn0[4] = RBS_ARG_AKN;         // ARG = "AKN"
  predefFrameAkn0[5] = RBS_FRM_LST;         // Frame end
  predefFrameAkn0[6] = '\0';                // Char arr end

  // Predefined AKN1 frame
  predefFrameAkn1 = new char[7];
  predefFrameAkn1[0] = RBS_FRM_FST;         // Frame start
  predefFrameAkn1[1] = RBS_SN_ONE;          // Frame S/N = '1'
  predefFrameAkn1[2] = RBS_LBL_HDS;         // LBL = "Heandshake message"
  predefFrameAkn1[3] = RBS_TYPE_AT;         // TYPE = "at"
  predefFrameAkn1[4] = RBS_ARG_AKN;         // ARG = "AKN"
  predefFrameAkn1[5] = RBS_FRM_LST;         // Frame end 
  predefFrameAkn1[6] = '\0';                // Char arr end

  // Predefined NAK frame
  predefFrameNak = new char[7];
  predefFrameNak[0] = RBS_FRM_FST;          // Frame start
  predefFrameNak[1] = RBS_SN_ZERO;          // Frame S/N = '0'
  predefFrameNak[2] = RBS_LBL_HDS;          // LBL = "Heandshake message"
  predefFrameNak[3] = RBS_TYPE_AT;          // TYPE = "at"
  predefFrameNak[4] = RBS_ARG_NAK;          // ARG = "NAK"
  predefFrameNak[5] = RBS_FRM_LST;          // Frame end 
  predefFrameNak[6] = '\0';                 // Char arr end

  // Predefined RDY frame
  predefFrameRdy = new char[7];
  predefFrameRdy[0] = RBS_FRM_FST;          // Frame start
  predefFrameRdy[1] = RBS_SN_ZERO;          // Frame S/N = '0'
  predefFrameRdy[2] = RBS_LBL_HDS;          // LBL = "Heandshake message"
  predefFrameRdy[3] = RBS_TYPE_AT;          // TYPE = "at"
  predefFrameRdy[4] = RBS_ARG_RDY;          // ARG = "RDY"
  predefFrameRdy[5] = RBS_FRM_LST;          // Frame end 
  predefFrameRdy[6] = '\0';                 // Char arr end

  // Predefined RST frame
  predefFrameRst = new char[7];
  predefFrameRst[0] = RBS_FRM_FST;          // Frame start
  predefFrameRst[1] = RBS_SN_ZERO;          // Frame S/N = '0'
  predefFrameRst[2] = RBS_LBL_HDS;          // LBL = "Heandshake message"
  predefFrameRst[3] = RBS_TYPE_AT;          // TYPE = "at"
  predefFrameRst[4] = RBS_ARG_RST;          // ARG = "RST"
  predefFrameRst[5] = RBS_FRM_LST;          // Frame end 
  predefFrameRst[6] = '\0';                 // Char arr end


  // Receiver related
  rx_state = RBSReceiverState::RX_START;    // Receiver FSM state
  acceptFrame = false;                      // is received frame accepted
  expectedRxFrameSn = RBS_SN_ZERO;          // expected sequence number of received frame
  rxNotReceivedCnt = 0;

  // Transmitter related
  tx_state = RBSTransmitterState::TX_START; // Transmitter FSM state
  txConfirmed = false;                      // is transmission confirmed  
  txFrameSn = RBS_SN_ZERO;                  // Sequence number of a Frame to transmit
  txAttCnt = 0;                             // counter for transmission attempts
  
  waitTmr = 0;                              // Timer to measure waiting time
  
}


// Destructor
//----------------------------------------------------------------------
RBSerial::~RBSerial(){
  
  delete[] parsedArgArr;
  delete[] predefFrameAkn0;
  delete[] predefFrameAkn1;
  delete[] predefFrameNak;
  delete[] predefFrameRdy;
  delete[] predefFrameRst;
  parsedArgArr = NULL;
  rbs_serial = NULL;
  Frame = NULL;
}


// Adding HardwareSerial object
//----------------------------------------------------------------------
void RBSerial::addSerial(HardwareSerial* serialPtr){
  rbs_serial = serialPtr;
  //rbs_serial->setTimeout(RBS_SER_TIMEOUT);
}


// Getters/Setters
//----------------------------------------------------------------------
bool RBSerial::getIsFrameReceived(){
  return frameReceived;
}

void RBSerial::setIsFrameReceived(bool isReceived){
  frameReceived = isReceived;
}


//----------------------------------------------------------------------
char RBSerial::getParsedSn(){
  return parsedSn;
}


void RBSerial::setParsedSn(char frmSn){
  parsedSn = frmSn;
}


//----------------------------------------------------------------------
char RBSerial::getParsedLbl(){
  return parsedLbl;
}

void RBSerial::setParsedLbl(char cmdLbl){
  parsedLbl = cmdLbl;
}


//----------------------------------------------------------------------
char RBSerial::getParsedType(){
  return parsedType;
}

void RBSerial::setParsedType(char cmdType){
  parsedType = cmdType;
}


//----------------------------------------------------------------------
char* RBSerial::getParsedArgArrPtr(){
  return parsedArgArr;
}

void RBSerial::setParsedArgArr(char* arr, int elCnt){
  if(elCnt <= RBS_ARG_LMAX)
  {
    for(int i = 0; i < RBS_ARG_LMAX; i++)
    {
      if(i < elCnt)
      {
        parsedArgArr[i] = arr[i];
      }
      else
      {
        parsedArgArr[i] = '\0';
      }
    }
  }
}

void RBSerial::setParsedArgArr(char argChar){
  parsedArgArr[0] = argChar;

  for(int i = 1; i < RBS_ARG_LMAX; i++)
  {
    parsedArgArr[i] = '\0';
  }

}







//----------------------------------------------------------------------
bool RBSerial::receiveFrame(){

  #ifdef DEBUG_MODE
    rbs_serial->println(F("RBSerial::receiveFrame() start"));
  #endif

  // delay to confirm serial received
  delay(RBS_SER_TIMEOUT);

  // Frame not received
  //frameReceived = false;
  
  // run finite state machine
  bool runFSM = true;

  // set starting state
  parser_state = RBSParserState::FRM_NOT_STARTED;
  
  while(runFSM)
  {
    #ifdef DEBUG_MODE
      rbs_serial->println(F("Run FSM"));
    #endif
    
    switch(parser_state)
    {
      //-----------------------------
      case RBSParserState::FRM_NOT_RECEIVED:
      {
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: FRM_NOT_RECEIVED start"));
        #endif
        
        frameReceived = false;
        runFSM = false; 

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: FRM_NOT_RECEIVED end"));
        #endif
        
        break;
      }

      //-----------------------------
      case RBSParserState::FRM_NOT_STARTED:
      {

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: FRM_NOT_STARTED start"));
        #endif
        
        // clear counters and data
        RBSerial::resetParser();
        
        if(rbs_serial->available())
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial available"));
          #endif
          
          incomingByte = (char)rbs_serial->read();

          #ifdef DEBUG_MODE
            rbs_serial->print(F("incoming byte/char = "));
            rbs_serial->println(incomingByte);
          #endif
          
          // next state
          if(incomingByte == RBS_FRM_FST)
          {
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: FRM_STARTED"));
            #endif
            
            parser_state = RBSParserState::FRM_STARTED;
          }
          else
          {
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: FRM_NOT_STARTED"));
            #endif
            
            parser_state = RBSParserState::FRM_NOT_STARTED;
          }          
        }
        else
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial NOT available"));
          #endif
          
          // next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: FRM_NOT_RECEIVED"));
          #endif
          
          parser_state = RBSParserState::FRM_NOT_RECEIVED;        
        } // end if(rbs_serial->available())
        
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: FRM_NOT_STARTED end"));
        #endif

        break;
      } 

      //-----------------------------
      case RBSParserState::FRM_RECEIVED:
      {
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: FRM_RECEIVED start"));
        #endif

        frameReceived = true;  // set flag
        runFSM = false;         // stop FSM

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: FRM_RECEIVED end"));
        #endif
        
        break;
      }

      //-----------------------------
      case RBSParserState::FRM_STARTED:
      {
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: FRM_STARTED start"));
        #endif
                
        if(rbs_serial->available())
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial available"));
          #endif
          
          incomingByte = (char)rbs_serial->read();

          #ifdef DEBUG_MODE
            rbs_serial->print(F("incoming byte/char = "));
            rbs_serial->println(incomingByte);
          #endif
          
          // next state
          if((incomingByte == RBS_SN_ZERO) || (incomingByte == RBS_SN_ONE))
          {
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: SN_RECEIVED"));
            #endif
            
            parsedSn = incomingByte;
            
            parser_state = RBSParserState::SN_RECEIVED;
          }
          else
          {
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: FRM_NOT_STARTED"));
            #endif
            
            parser_state = RBSParserState::FRM_NOT_STARTED;
            
          }         
        }
        else
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial NOT available"));
          #endif
          
          // next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: FRM_NOT_RECEIVED"));
          #endif

          parser_state = RBSParserState::FRM_NOT_RECEIVED;
        }
        
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: FRM_STARTED end"));
        #endif
        
        break;
      }


      //-----------------------------
      case RBSParserState::SN_RECEIVED:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: SN_RECEIVED start"));
        #endif        

        if(rbs_serial->available()){
          
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial available"));
          #endif
          
          incomingByte = (char)rbs_serial->read();
          
          #ifdef DEBUG_MODE
            rbs_serial->print(F("incoming byte/char = "));
            rbs_serial->println(incomingByte);
          #endif
          
          // next state
          if((incomingByte == RBS_FRM_FST) || (incomingByte == RBS_FRM_LST))
          {
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: FRM_NOT_STARTED"));
            #endif
            
            parser_state = RBSParserState::FRM_NOT_STARTED;
                        
          }
          else
          {

            parsedLbl = incomingByte;
            
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: LBL_RECEIVED"));
            #endif
                                  
            parser_state = RBSParserState::LBL_RECEIVED;
            
          }                 
          
        }
        else{
          
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial NOT available"));
          #endif
          
          // next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: FRM_NOT_RECEIVED"));
          #endif
          
          parser_state = RBSParserState::FRM_NOT_RECEIVED;          
        }


        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: SN_RECEIVED end"));
        #endif  
        
        break;
      }

      //-----------------------------
      case RBSParserState::LBL_RECEIVED:
      {
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: LBL_RECEIVED start"));
        #endif
                
        if(rbs_serial->available())
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial available"));
          #endif
          
          incomingByte = (char)rbs_serial->read();

          #ifdef DEBUG_MODE
            rbs_serial->print(F("incoming byte/char = "));
            rbs_serial->println(incomingByte);
          #endif
          
          switch(incomingByte)
          {
            case RBS_TYPE_GET:
            {
              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: TYPE_GET_RECEIVED"));
              #endif
              
              parsedType = RBS_TYPE_GET;
              parser_state = RBSParserState::TYPE_GET_RECEIVED;
              break;
            }
            case RBS_TYPE_SET:
            {
              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: TYPE_SET_RECEIVED"));
              #endif
                            
              parsedType = RBS_TYPE_SET;
              parser_state = RBSParserState::TYPE_SET_RECEIVED;              
              break;
            }
            case RBS_TYPE_AT:
            {
              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: TYPE_AT_RECEIVED"));
              #endif
                            
              parsedType = RBS_TYPE_AT;
              parser_state = RBSParserState::TYPE_AT_RECEIVED;                  
              break;
            }
            default:
            {
              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: FRM_NOT_STARTED"));
              #endif
                            
              parser_state = RBSParserState::FRM_NOT_STARTED;
              break;
            }
          }          
        }
        else
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial NOT available"));
          #endif
          
          // next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: FRM_NOT_RECEIVED"));
          #endif
          
          parser_state = RBSParserState::FRM_NOT_RECEIVED;
        }
        
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: LBL_RECEIVED end"));
        #endif
        
        break;
      }

      //-----------------------------
      case RBSParserState::TYPE_AT_RECEIVED:
      {
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TYPE_AT_RECEIVED start"));
        #endif
                
        if(rbs_serial->available())
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial available"));
          #endif
          
          incomingByte = (char)rbs_serial->read();

          #ifdef DEBUG_MODE
            rbs_serial->print(F("incoming byte/char = "));
            rbs_serial->println(incomingByte);
          #endif

          // next state
          if(incomingByte == RBS_FRM_FST || incomingByte == RBS_FRM_LST)
          {
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: FRM_NOT_STARTED"));
            #endif  
                      
            parser_state = RBSParserState::FRM_NOT_STARTED;
          }
          else
          {                       
            parsedArgArr[currArgIdx] = incomingByte;
            currArgIdx++;
            parser_state = RBSParserState::ARG_BYTE_RECEIVED;

            #ifdef DEBUG_MODE
              rbs_serial->print(F("next idx of val arr = "));
              rbs_serial->println(currArgIdx);
            #endif               

            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: ARG_BYTE_RECEIVED"));
            #endif              
          }         
        }
        else
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial NOT available"));
          #endif
          
          // next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: FRM_NOT_RECEIVED"));
          #endif
          
          parser_state = RBSParserState::FRM_NOT_RECEIVED;
        }
        
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TYPE_AT_RECEIVED end"));
        #endif
        
        break;
      }

      //-----------------------------
      case RBSParserState::TYPE_GET_RECEIVED:
      {
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TYPE_GET_RECEIVED start"));
        #endif
        
        if(rbs_serial->available())
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial available"));
          #endif
          
          incomingByte = (char)rbs_serial->read();

          #ifdef DEBUG_MODE
            rbs_serial->print(F("incoming byte/char = "));
            rbs_serial->println(incomingByte);
          #endif
          
          if(incomingByte == RBS_FRM_LST)
          {
            // next state
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: FRM_RECEIVED"));
            #endif            
            
            parser_state = RBSParserState::FRM_RECEIVED;
          }
          else
          {
            // next state
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: FRM_NOT_STARTED"));
            #endif                 
            parser_state = RBSParserState::FRM_NOT_STARTED;
          }          
        }
        else
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial NOT available"));
          #endif
          
          // next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: FRM_NOT_RECEIVED"));
          #endif
          
          parser_state = RBSParserState::FRM_NOT_RECEIVED;
        }
        
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TYPE_GET_RECEIVED end"));
        #endif
        
        break;
      }

      //-----------------------------
      case RBSParserState::TYPE_SET_RECEIVED:
      {
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TYPE_SET_RECEIVED start"));
        #endif
                
        if(rbs_serial->available())
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial available"));
          #endif
          
          incomingByte = (char)rbs_serial->read();

          #ifdef DEBUG_MODE
            rbs_serial->print(F("incoming byte/char = "));
            rbs_serial->println(incomingByte);
          #endif

          // next state
          if(incomingByte == RBS_FRM_FST || incomingByte == RBS_FRM_LST)
          {
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: FRM_NOT_STARTED"));
            #endif   
                    
            parser_state = RBSParserState::FRM_NOT_STARTED;
          }
          else
          {
            parsedArgArr[currArgIdx] = incomingByte;
            currArgIdx++;
            parser_state = RBSParserState::ARG_BYTE_RECEIVED;
            
            #ifdef DEBUG_MODE
              rbs_serial->print(F("next idx of val arr = "));
              rbs_serial->println(currArgIdx);
            #endif               

            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: ARG_BYTE_RECEIVED"));
            #endif     
            
          }   
        }
        else
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial NOT available"));
          #endif
          
          // next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: FRM_NOT_RECEIVED"));
          #endif
          
          parser_state = RBSParserState::FRM_NOT_RECEIVED;
        }

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TYPE_SET_RECEIVED end"));
        #endif
        
        break;
      }

      //-----------------------------
      case RBSParserState::ARG_BYTE_RECEIVED:
      {
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: ARG_BYTE_RECEIVED start"));
        #endif
                
        if(rbs_serial->available())
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial available"));
          #endif

          if(currArgIdx >= RBS_ARG_LMAX)
          {
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Max length of command value exceeded"));
            #endif

            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: FRM_NOT_STARTED"));
            #endif
            
            parser_state = RBSParserState::FRM_NOT_STARTED;
          }
          else
          {     
            incomingByte = (char)rbs_serial->read();
  
            #ifdef DEBUG_MODE
              rbs_serial->print(F("incoming byte/char = "));
              rbs_serial->println(incomingByte);
            #endif

            if(incomingByte == RBS_FRM_LST)
            {
              // next state
              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: FRM_RECEIVED"));
              #endif
              
              parser_state = RBSParserState::FRM_RECEIVED;
            }
            else
            {
              // next state
              parsedArgArr[currArgIdx] = incomingByte;
              currArgIdx++;
              parser_state = RBSParserState::ARG_BYTE_RECEIVED;

              #ifdef DEBUG_MODE
                rbs_serial->print(F("next idx of val arr = "));
                rbs_serial->println(currArgIdx);
              #endif  

              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: ARG_BYTE_RECEIVED"));
              #endif
            }              
          }
        }
        else
        {
          #ifdef DEBUG_MODE
            rbs_serial->println(F("serial NOT available"));
          #endif
          
          // next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: FRM_NOT_RECEIVED"));
          #endif
          
          parser_state = RBSParserState::FRM_NOT_RECEIVED;
        }

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: ARG_BYTE_RECEIVED end"));
        #endif        

        break;
      }

      default:
      {
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: undefined state start"));
        #endif  
          
        parser_state = RBSParserState::FRM_NOT_STARTED;

        #ifdef DEBUG_MODE
          rbs_serial->println(F("Next state: FRM_NOT_STARTED"));
        #endif

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: undefined state end"));
        #endif 
         
        break;
      }
    } // end switch
  } // end while

  #ifdef DEBUG_MODE
    rbs_serial->print(F("RBSerial::receiveFrame() end, return value = "));
    rbs_serial->println(frameReceived);
  #endif

  return frameReceived;
}


//----------------------------------------------------------------------
bool RBSerial::receiveARQ(){

  #ifdef DEBUG_MODE
    rbs_serial->println(F("RBSerial::receiveARQ() start"));
  #endif
  
  // run finite state machine
  bool runFSM = true;

  // set starting state
  rx_state = RBSReceiverState::RX_START;
  
  while(runFSM){

    #ifdef DEBUG_MODE
      rbs_serial->println(F("Run FSM"));
    #endif
    
    switch(rx_state){

      
      //-----------------------------
      case RBSReceiverState::RX_ACCEPT_FRAME:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_ACCEPT_FRAME start"));
        #endif 

        acceptFrame = true;

        // Next state
        if(parsedSn == RBS_SN_ZERO){
          
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: RX_SEND_AKN_1"));
          #endif  

          rx_state = RBSReceiverState::RX_SEND_AKN_1; 
          
        }
        else if(parsedSn == RBS_SN_ONE){
  
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: RX_SEND_AKN_0"));
          #endif  

          rx_state = RBSReceiverState::RX_SEND_AKN_0; 
          
        }
        else{

          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: RX_DISCARD_FRAME"));
          #endif 

          rx_state = RBSReceiverState::RX_DISCARD_FRAME;             
        }

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_ACCEPT_FRAME end"));
        #endif   

        break;
      }


      //-----------------------------
      case RBSReceiverState::RX_END:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_END start"));
        #endif 

        // Stop FSM
        runFSM = false;

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_END end"));
        #endif 

        break;
      }



      //-----------------------------
      case RBSReceiverState::RX_DISCARD_FRAME:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_DISCARD_FRAME start"));
        #endif 

        acceptFrame = false;

        // next state
        rx_state = RBSReceiverState::RX_END;
        

        #ifdef DEBUG_MODE
          rbs_serial->println(F("Next state: RX_END"));
        #endif

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_DISCARD_FRAME end"));
        #endif

        break;
      }

      //-----------------------------
      case RBSReceiverState::RX_RST_RECEIVED:{
        
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_RST_RECEIVED start"));
        #endif 

        RBSerial::resetRX();
        RBSerial::resetTX();

        // 2026-08-01
        //acceptFrame = true;

        // next state
        // ORIGINAL
        //rx_state = RBSReceiverState::RX_DISCARD_FRAME;

        // 2026-10-01
        rx_state = RBSReceiverState::RX_START;
        
        // 2026-08-01
        //rx_state = RBSReceiverState::RX_END;

        #ifdef DEBUG_MODE
          rbs_serial->println(F("Next state: RX_DISCARD_FRAME"));
        #endif


        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_RST_RECEIVED end"));
        #endif 
        
        break;
      }

      //-----------------------------
      case RBSReceiverState::RX_SEND_AKN_0:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_SEND_AKN_0 start"));
        #endif 

        // Send predefined Akn sequence number = '0' frame
        RBSerial::sendFrame(predefFrameAkn0); 
        #ifdef DOUBLE_AKN
          RBSerial::sendFrame(predefFrameAkn0); 
        #endif
        
        // Set expected next frame sequence number = '0'
        expectedRxFrameSn = RBS_SN_ZERO;

        // next state
        rx_state = RBSReceiverState::RX_END;

        #ifdef DEBUG_MODE
          rbs_serial->println(F("Next state: RX_END"));
        #endif        

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_SEND_AKN_0 end"));
        #endif 
        
        break;
      }


      //-----------------------------
      case RBSReceiverState::RX_SEND_AKN_1:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_SEND_AKN_1 start"));
        #endif 

        // Send predefined Akn sequence number = '1' frame
        RBSerial::sendFrame(predefFrameAkn1);
        #ifdef DOUBLE_AKN
          RBSerial::sendFrame(predefFrameAkn1); 
        #endif
        
        // Set expected next frame sequence number = '1'
        expectedRxFrameSn = RBS_SN_ONE;

        // next state
        rx_state = RBSReceiverState::RX_END;

        #ifdef DEBUG_MODE
          rbs_serial->println(F("Next state: RX_END"));
        #endif        

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_SEND_AKN_1 end"));
        #endif 
        
        break;
      }      

      
      //-----------------------------
      case RBSReceiverState::RX_START:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_START start"));
        #endif  

        //RBSerial::resetParser();
        acceptFrame = false;

        // 2026-10-07
        //rbs_serial->begin(115200);
        //rbs_serial->flush();

        // next state
        if(expectedRxFrameSn == RBS_SN_ZERO){
          
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: RX_WAIT_FRAME_0"));
          #endif 

          rx_state = RBSReceiverState::RX_WAIT_FRAME_0;
          
        }
        else if(expectedRxFrameSn == RBS_SN_ONE){

          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: RX_WAIT_FRAME_1"));
          #endif 

          rx_state = RBSReceiverState::RX_WAIT_FRAME_1;          
          
        }
        else{

          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: RX_DISCARD_FRAME"));
          #endif 

          rx_state = RBSReceiverState::RX_DISCARD_FRAME;     
          
        }
                
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_START end"));
        #endif   
        
        break;
      }


      //-----------------------------
      case RBSReceiverState::RX_WAIT_FRAME_0:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_WAIT_FRAME_0 start"));
        #endif 

        //start timer
        waitTmr = millis();

        while(millis() - waitTmr <= RBS_RESP_WAIT_TIME){

          RBSerial::receiveFrame();

          if(frameReceived && RBSerial::checkRST()){
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: RX_RST_RECEIVED"));
            #endif 
  
            rx_state = RBSReceiverState::RX_RST_RECEIVED;  

            break;              
          }

          if(frameReceived && (parsedLbl != RBS_LBL_HDS) && (parsedSn == RBS_SN_ZERO)){
            
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: RX_ACCEPT_FRAME"));
            #endif 
  
            rx_state = RBSReceiverState::RX_ACCEPT_FRAME;  

            break;
          }
          else{
  
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: RX_DISCARD_FRAME"));
            #endif 
  
            rx_state = RBSReceiverState::RX_DISCARD_FRAME;              
          }
        }

        
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_WAIT_FRAME_0 end"));
        #endif  
        
        break;
      }


     //-----------------------------
      case RBSReceiverState::RX_WAIT_FRAME_1:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_WAIT_FRAME_1 start"));
        #endif  
        
        //start timer
        waitTmr = millis();

        while(millis() - waitTmr <= RBS_RESP_WAIT_TIME){
        
          RBSerial::receiveFrame();

          if(frameReceived && RBSerial::checkRST()){
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: RX_RST_RECEIVED"));
            #endif 
  
            rx_state = RBSReceiverState::RX_RST_RECEIVED;  

            break;              
          }
            
          if(frameReceived && (parsedLbl != RBS_LBL_HDS) && (parsedSn == RBS_SN_ONE)){
            
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: RX_ACCEPT_FRAME"));
            #endif 
  
            rx_state = RBSReceiverState::RX_ACCEPT_FRAME;  

            break;
          }
          else{
  
            #ifdef DEBUG_MODE
              rbs_serial->println(F("Next state: RX_DISCARD_FRAME"));
            #endif 
  
            rx_state = RBSReceiverState::RX_DISCARD_FRAME;  
            
          }
        }

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: RX_WAIT_FRAME_1 end"));
        #endif  
        
        break;
      }


      //-----------------------------
      default:{
        
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: RX_DISCARD_FRAME"));
          #endif 

          rx_state = RBSReceiverState::RX_DISCARD_FRAME; 
           
        break;
      }    
    }
  }

  #ifdef DEBUG_MODE
    rbs_serial->print(F("RBSerial::receiveARQ() end, Rerurn bool acceptFrame="));
    rbs_serial->println(acceptFrame)
  #endif
  
  return acceptFrame;
}


//----------------------------------------------------------------------
bool RBSerial::transmitARQ(){

  #ifdef DEBUG_MODE
    rbs_serial->println(F("RBSerial::transmitARQ() start"));
  #endif
  
 
  // run finite state machine
  bool runFSM = true;

  // set starting state
  tx_state = RBSTransmitterState::TX_START;
  
  while(runFSM){

    switch(tx_state){

      //-----------------------------
      case RBSTransmitterState::TX_CONFIRMED:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_CONFIRMED start"));
        #endif 

        // Dealloc frame
        RBSerial::deallocFrame();

        // Set flag
        txConfirmed = true;

        // Stop FSM
        runFSM = false;

        // Update sequence number for next frame to be transmitted
        if(txFrameSn == RBS_SN_ZERO){

          txFrameSn = RBS_SN_ONE;
        }
        else if(txFrameSn == RBS_SN_ONE){
          
          txFrameSn = RBS_SN_ZERO;
        }
        else{
          runFSM = true;
          //next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: TX_FAIL"));
          #endif  

          tx_state = RBSTransmitterState::TX_FAIL;   
        }

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_CONFIRMED end"));
        #endif         

        break;
      }

       //-----------------------------
      case RBSTransmitterState::TX_FAIL:{


        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_FAIL start"));
        #endif 

        // Dealloc frame
        RBSerial::deallocFrame();

        // Set flag
        txConfirmed = false;

        // Stop FSM
        runFSM = false; 

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_FAIL end"));
        #endif        

        break;
      }


      //-----------------------------
      case RBSTransmitterState::TX_RST_RECEIVED:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_RST_RECEIVED start"));
        #endif 

        RBSerial::resetRX();
        RBSerial::resetTX();

        //next state
        #ifdef DEBUG_MODE
          rbs_serial->println(F("Next state: TX_FAIL"));
        #endif  

        // ORIGINAL
        // tx_state = RBSTransmitterState::TX_FAIL;

        // 2026-10-07
        tx_state = RBSTransmitterState::TX_START;
        
        //2026-08-02
        //txFrameSn = RBS_SN_ZERO;
        //tx_state = RBSTransmitterState::TX_START;

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_RST_RECEIVED end"));
        #endif 
        
        break;
      }


      //-----------------------------
      case RBSTransmitterState::TX_SEND_FRAME_0:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_SEND_FRAME_0 start"));
        #endif 

        if(txAttCnt <= RBS_ARQ_ATT_MAX){
          
          RBSerial::updateFrameSn(RBS_SN_ZERO);
          RBSerial::sendFrame(Frame);
          txAttCnt++;

          //next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: TX_WAIT_AKN_1"));
          #endif  

          tx_state = RBSTransmitterState::TX_WAIT_AKN_1;
                    
        }
        else{

          //next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: TX_FAIL"));
          #endif  

          tx_state = RBSTransmitterState::TX_FAIL;         
        }

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_SEND_FRAME_0 end"));
        #endif    

        break;
      }


      //-----------------------------
      case RBSTransmitterState::TX_SEND_FRAME_1:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_SEND_FRAME_1 start"));
        #endif 

        if(txAttCnt <= RBS_ARQ_ATT_MAX){
          
          RBSerial::updateFrameSn(RBS_SN_ONE);
          RBSerial::sendFrame(Frame);
          txAttCnt++;

          //next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: TX_WAIT_AKN_0"));
          #endif  

          tx_state = RBSTransmitterState::TX_WAIT_AKN_0;
                    
        }
        else{

          //next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: TX_FAIL"));
          #endif  

          tx_state = RBSTransmitterState::TX_FAIL;         
        }

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_SEND_FRAME_1 end"));
        #endif   

        break;
      }


      //-----------------------------
      case RBSTransmitterState::TX_START:{


        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_START start"));
        #endif 
        
        RBSerial::allocFrame();

        // Clear flag
        txConfirmed = false;

        //Reset counter
        txAttCnt = 0;
        
        // Next state
        if(txFrameSn == RBS_SN_ZERO){

          //next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: TX_SEND_FRAME_0"));
          #endif  

          tx_state = RBSTransmitterState::TX_SEND_FRAME_0;                 
        }
        else if(txFrameSn == RBS_SN_ONE){
          
          //next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: TX_SEND_FRAME_1"));
          #endif  

          tx_state = RBSTransmitterState::TX_SEND_FRAME_1;             
        }
        else{

          //next state
          #ifdef DEBUG_MODE
            rbs_serial->println(F("Next state: TX_FAIL"));
          #endif  

          tx_state = RBSTransmitterState::TX_FAIL;   
        }

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_START end"));
        #endif  

        break;
      }

      //-----------------------------
      case RBSTransmitterState::TX_WAIT_AKN_0:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_WAIT_AKN_0 start"));
        #endif 

        //start timer
        waitTmr = millis();

        while(millis() - waitTmr <= RBS_RESP_WAIT_TIME){

          // if some frame received
          if(RBSerial::receiveFrame()){

            if(RBSerial::checkRST()){
              
              //next state
              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: TX_RST_RECEIVED"));
              #endif  

              tx_state = RBSTransmitterState::TX_RST_RECEIVED; 
              break;
            }

            // if received frame is akn '0' 
            if(RBSerial::checkLbl(RBS_LBL_HDS) && RBSerial::checkArgChar(RBS_ARG_AKN) && RBSerial::checkSn(RBS_SN_ZERO)){

              //next state
              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: TX_CONFIRMED"));
              #endif  

              tx_state = RBSTransmitterState::TX_CONFIRMED; 

              break;
            }
            else{

              //next state
              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: TX_SEND_FRAME_1"));
              #endif  

              tx_state = RBSTransmitterState::TX_SEND_FRAME_1; 
            }            
          } // End if(RBSerial::receiveFrame())        
        } // End While

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TTX_WAIT_AKN_0 end"));
        #endif 

        break;
      }

      //-----------------------------
      case RBSTransmitterState::TX_WAIT_AKN_1:{

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TX_WAIT_AKN_1 start"));
        #endif 

        //start timer
        waitTmr = millis();

        while(millis() - waitTmr <= RBS_RESP_WAIT_TIME){

          // if some frame received
          if(RBSerial::receiveFrame()){

            if(RBSerial::checkRST()){
              
              //next state
              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: TX_RST_RECEIVED"));
              #endif  

              tx_state = RBSTransmitterState::TX_RST_RECEIVED; 
              break;
            }

            // if received frame is akn '1' 
            if(RBSerial::checkLbl(RBS_LBL_HDS) && RBSerial::checkArgChar(RBS_ARG_AKN) && RBSerial::checkSn(RBS_SN_ONE)){

              //next state
              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: TX_CONFIRMED"));
              #endif  

              tx_state = RBSTransmitterState::TX_CONFIRMED; 

              break;
            }
            else{

              //next state
              #ifdef DEBUG_MODE
                rbs_serial->println(F("Next state: TX_SEND_FRAME_0"));
              #endif  

              tx_state = RBSTransmitterState::TX_SEND_FRAME_0; 
            }           
          } // End if(RBSerial::receiveFrame())        
        } // End wjile

        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: TTX_WAIT_AKN_1 end"));
        #endif 

        break;
      }

      //-----------------------------
      default:{
        
        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: Default start"));
        #endif 

        //next state
        #ifdef DEBUG_MODE
          rbs_serial->println(F("Next state: TX_FAIL"));
        #endif  

        tx_state = RBSTransmitterState::TX_FAIL;   


        #ifdef DEBUG_MODE
          rbs_serial->println(F("State: Default end"));
        #endif 
        break;
      }      
    }    
  }

  #ifdef DEBUG_MODE
    rbs_serial->print(F("RBSerial::transmitARQ() end. Return bool txConfirmed = "));
    rbs_serial->println(txConfirmed);
  #endif

  return txConfirmed;
}

//----------------------------------------------------------------------
void RBSerial::transmitRdy(){

  RBSerial::sendFrame(predefFrameRdy); 
  
}

//----------------------------------------------------------------------
void RBSerial::sendParsedFrame(){

  RBSerial::allocFrame();
    
  rbs_serial->println(Frame);
  rbs_serial->flush();

  RBSerial::deallocFrame();
}


//----------------------------------------------------------------------
void RBSerial::sendFrame(char* arr){
  rbs_serial->println(arr);
  rbs_serial->flush();
}


//----------------------------------------------------------------------
bool RBSerial::commInquiryData(char dataLbl){
  
  // Send Frame with inquiry and receive Frame with Answer, ARQ Stop&Wait + reset RX/TX on error
  // Frame with answer will be parsed into corresponding member variables

  // Fill frame fields with request for sensor data
  RBSerial::setParsedLbl(dataLbl);          // label of data to be inquiried
  RBSerial::setParsedType(RBS_TYPE_GET);    // "get" type message
  
  bool received = false;

  // try to communicate few times
  // High level comm protocol
  for(int i = 0; i < RBS_COMM_TX_ATT_MAX; i++){

    // if request has been tranmitted
    if(RBSerial::transmitARQ()){
  
      received = RBSerial::receiveARQ(); // receive reply
  
      // if reply has been received
      if(received){
        break;
      }
    }
    else{
      // Reset comm on both sides
      RBSerial::resetComm();    
    }
  }

  //2026-08-01
  //if(!received){
  //  RBSerial::resetComm();
  //}

  return received;  
}


//----------------------------------------------------------------------
bool RBSerial::commSendCommand(char dataLbl, int dataVal){

  // Overloaded wrap for commSendParsedFrame(), int dataVal
  
  // Fill frame fields with stop command
  RBSerial::setParsedLbl(dataLbl);
  RBSerial::setParsedType(RBS_TYPE_SET);
  RBSerial::intToArgArr(dataVal); 

  return commSendParsedFrame();
}

bool RBSerial::commSendCommand(char dataLbl, double dataVal){
  
  // Overloaded wrap for commSendParsedFrame(), double dataVal
  
  // Fill frame fields with stop command
  RBSerial::setParsedLbl(dataLbl);
  RBSerial::setParsedType(RBS_TYPE_SET);
  RBSerial::dblToArgArr(dataVal); 

  return commSendParsedFrame();
}

bool RBSerial::commSendCommand(char dataLbl, unsigned long dataVal){

  // Overloaded wrap for commSendParsedFrame(), unsigned long dataVal

  // Fill frame fields with stop command
  RBSerial::setParsedLbl(dataLbl);
  RBSerial::setParsedType(RBS_TYPE_SET);
  RBSerial::ulongToArgArr(dataVal); 

  return commSendParsedFrame();  
}


bool RBSerial::commSendCommand(char dataLbl, char dataVal){

  // Overloaded wrap for commSendParsedFrame(), for single-char dataVal

  // Fill frame fields with stop command
  RBSerial::setParsedLbl(dataLbl);
  RBSerial::setParsedType(RBS_TYPE_SET);
  RBSerial::setParsedArgArr(dataVal);

  return commSendParsedFrame();   
}


bool RBSerial::commSendCommand(char dataLbl, char* dataVal, int elCnt){

  // Overloaded wrap for commSendParsedFrame(), for char array dataVal

  // Fill frame fields with stop command
  RBSerial::setParsedLbl(dataLbl);
  RBSerial::setParsedType(RBS_TYPE_SET);
  RBSerial::setParsedArgArr(dataVal, elCnt); 

  return commSendParsedFrame(); 
}




//----------------------------------------------------------------------    
bool RBSerial::commReceive(){
  // For master/slave: Receive frame, reset both parties RX/TX if long time no incoming frames 

  bool received = false;

  if(rxNotReceivedCnt > RBS_COMM_RX_ATT_MAX){

    rxNotReceivedCnt = 0;
    // Reset comm on both sides
    RBSerial::resetComm();      
  }

  received = receiveARQ();

  if(received){
    rxNotReceivedCnt=0;
  }
  else{
    rxNotReceivedCnt++;
  }

  return received;  
}



// Aux
//----------------------------------------------------------------------
bool RBSerial::commSendParsedFrame(){

  // For master: Send Frame with set-type command, ARQ Stop&Wait + reset both parties RX/TX on error

  bool transmitted = false;
  
  for(int i = 0; i < RBS_COMM_TX_ATT_MAX; i++){

    transmitted = RBSerial::transmitARQ();
    if(transmitted){
      break;
    }
    else{
        // Reset comm on both sides
        RBSerial::resetComm();   
    }
  }

 return transmitted;   
}


//----------------------------------------------------------------------
void RBSerial::resetRX(){

  //rx_state = RBSReceiverState::RX_DISCARD_FRAME;    // Receiver FSM state
  acceptFrame = false;                      // is received frame accepted
  expectedRxFrameSn = RBS_SN_ZERO;          // expected sequence number of received frame

  // try 2026-07-31
  //RBSerial::sendFrame(predefFrameAkn0);
  //RBSerial::sendFrame(predefFrameAkn0);
  
  // try 2026-07-31
  //receiveFrame();
}


//----------------------------------------------------------------------
void RBSerial::resetTX(){
   
  //tx_state = RBSTransmitterState::TX_FAIL; // Transmitter FSM state
  txConfirmed = false;                      // is transmission confirmed  
  txFrameSn = RBS_SN_ZERO;                  // Sequence number of a Frame to transmit
  txAttCnt = 0;                             // counter for transmission attempts 

  // try 2026-07-31
  //RBSerial::sendFrame(predefFrameAkn0);
  //RBSerial::sendFrame(predefFrameAkn0);  

  // try 2026-07-31
  //RBSerial::updateFrameSn(RBS_SN_ZERO);     // update current frame
  //RBSerial::sendParsedFrame();
}


//----------------------------------------------------------------------
void RBSerial::sendRST(){

  rbs_serial->println(predefFrameRst);
  rbs_serial->flush();
  rbs_serial->println(predefFrameRst);
  rbs_serial->flush();  
}


//----------------------------------------------------------------------
void RBSerial::resetComm(){

  // try 2026-07-31
  //txFrameSn = RBS_SN_ZERO;
  //expectedRxFrameSn = RBS_SN_ZERO;
  //parsedSn = RBS_SN_ZERO;
  
  // Reset comm on both sides
  RBSerial::sendRST();
  RBSerial::resetRX();
  RBSerial::resetTX(); 
  delay(5*RBS_SER_TIMEOUT);   
  
}

//----------------------------------------------------------------------
void RBSerial::clearArgArr(){
  
  for(int i = 0; i < RBS_ARG_LMAX; i++)
  {
    parsedArgArr[i] = '\0';
  }
}


//----------------------------------------------------------------------
void RBSerial::resetParser(){
  
  frameReceived = false;
  currArgIdx = 0;
  parsedSn = '\0';
  parsedLbl = '\0';
  parsedType = '\0';
  RBSerial::clearArgArr();
}


//----------------------------------------------------------------------
void RBSerial::allocFrame(){

 if(parsedType == RBS_TYPE_GET){
    Frame = new char[6];
    Frame[0] = RBS_FRM_FST;
    Frame[1] = parsedSn;
    Frame[2] = parsedLbl;
    Frame[3] = RBS_TYPE_GET;
    Frame[4] = RBS_FRM_LST;
    Frame[5] = '\0';
  }
  else{
    // find idx of first '\0' in the parsedArgArr
    // = number of chars before first '\0'
    char* pch = strchr(parsedArgArr, '\0');
    int chars_cnt = pch - parsedArgArr;
    int pckSize = RBS_FRM_SIZEMIN + chars_cnt + 1;

    Frame = new char[pckSize]; 
    Frame[0] = RBS_FRM_FST;
    Frame[1] = parsedSn;
    Frame[2] = parsedLbl;
    Frame[3] = parsedType;
    for(int i = 4; i < chars_cnt + 4; i++)
    {
      Frame[i] = parsedArgArr[i - 4];
    }
    Frame[pckSize - 2] = RBS_FRM_LST;
    Frame[pckSize - 1] = '\0';     
  }
}


//----------------------------------------------------------------------
void RBSerial::updateFrameSn(char frmSn){
  
  Frame[1] = frmSn;
}


//----------------------------------------------------------------------
void RBSerial::deallocFrame(){
  
  delete[] Frame;
  Frame = NULL;
}


//----------------------------------------------------------------------
void RBSerial::dblToArgArr(double val){

  //String tmp = String(val, RBS_DBL_PREC);
  //tmp.toCharArray(parsedArgArr, RBS_ARG_LMAX);
  KB_dtoa(val, parsedArgArr, RBS_ARG_LMAX, RBS_DBL_PREC);

  

}


void RBSerial::intToArgArr(int val){
  itoa(val, parsedArgArr, 10); // 10 not a size. It is the base value
  //String tmp = String(val);
  //tmp.toCharArray(parsedArgArr, RBS_ARG_LMAX);
}


void RBSerial::ulongToArgArr(unsigned long val){
  ltoa(val, parsedArgArr, 10); // 10 not a size. It is the base value
}


int RBSerial::argArrToInt(){
  return atoi(parsedArgArr);
}


double RBSerial::argArrToDbl(){
  //String tmp(parsedArgArr);
  //return (double) tmp.toFloat();
  return (double) atof(parsedArgArr);
  
}


unsigned long RBSerial::argArrToULong(){
  return (unsigned long) atol(parsedArgArr);
}


bool RBSerial::checkLbl(char testLbl){
  if(testLbl == parsedLbl)
  {
    return true;
  }
  else
  {
    return false;
  }
}


bool RBSerial::checkSn(char testSn){
  
  if(testSn == parsedSn){
    return true;
  }
  else{
    return false;
  }  
}


bool RBSerial::checkType(char testType){
  
  if(testType == parsedType){
    return true;
  }
  else{
    return false;
  }
  
}


bool RBSerial::checkArgChar(char testArgChar){
  if(testArgChar == parsedArgArr[0]){
    return true;
  }
  else{
    return false;
  }
}


bool RBSerial::checkArgString(String testArgString){
  
  if(testArgString == String(parsedArgArr)){
    return true;
  }
  else{
    return false;
  }
  
}

bool RBSerial::checkRST(){

  if(RBSerial::checkLbl(RBS_LBL_HDS) && RBSerial::checkArgChar(RBS_ARG_RST)){
    return true;
  }
  else{
    return false;
  }
  
}


//----------------------------------------------------------------------
void RBSerial::clearSerialBuffer() {
 while (rbs_serial->available()) {
   rbs_serial->read(); // Read and discard each byte
 }
}


//----------------------------------------------------------------------
void RBSerial::disp(){
  if(*rbs_serial)
  {
    rbs_serial->println(F("RBSerial object status:"));
    rbs_serial->print(F("Frame received: "));
    rbs_serial->println(frameReceived);
    rbs_serial->print(F("Frame sequence number: "));
    rbs_serial->println(parsedSn);
    rbs_serial->print(F("Parameter label: "));
    rbs_serial->println(parsedLbl);
    rbs_serial->print(F("Command type: "));
    rbs_serial->println(parsedType);
    rbs_serial->print(F("Command value: "));
    rbs_serial->println(parsedArgArr);
  }
}
