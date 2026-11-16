//==============================================================================================
//Version 2.1 2026-10-08
// - RBS_transmitRdy() added
//
//   This function is to send READY confirmation:
//   It means that receiver in not busy and ready to receive new data or command
//   (Not the same as RBS ARQ protocol AKN.
//   AKN only confirms receiving of frame, but doesnt confirm that receiver is ready)
//==============================================================================================
Init_lib_RBS:

// Sub to keep all related variables in separate file
// RBS_INIT must be called in main programm

// Communication protocol related parameters
// Must be in accordance with same parameters programmed into MCU
//******************
//* Same as in MCU *
//******************
RBS_DOUBLE_AKN = true       // Send AKN twice

RBS_RESP_WAIT_TIME = 100     // ms, default max wait time for responce  100 default
RBS_SER_TIMEOUT = 10           // ms to wait for serial data            20 default
RBS_ARQ_ATT_MAX = 5
RBS_COMM_TX_ATT_MAX = 100
RBS_COMM_RX_ATT_MAX = 500

RBS_ARG_LMAX = 16               // max number of bytes for received value 
RBS_FRM_SIZEMIN = 5             // min size of packet

RBS_FRM_FST = "["               // 0x5B "[" Opening bracket 
RBS_FRM_LST = "]"               // 0x5D "]" Closing bracket

RBS_TYPE_GET = "?"              // 0x3F "?" request for value
RBS_TYPE_SET = "="              // 0x3D "=" set value
RBS_TYPE_AT = "@"               // 0x3D "=" set value
RBS_LBL_HDS = "#"          // Handshake message
RBS_SN_ZERO = "0"
RBS_SN_ONE = "1"
RBS_ARG_AKN = "+"          // Argument for robot - exec motor programm
RBS_ARG_NAK = "-"          // Argument for robot - go forward
RBS_ARG_RDY = ">"          // Argument for robot - go forward
RBS_ARG_RST = "!"          // Argument for robot - go forward

RBS_DBL_PREC = 2            // precision for double type when conwert to string

// Parser FSM states
RBS_PARS_FRM_NOT_RECEIVED = 0
RBS_PARS_FRM_NOT_STARTED =  1
RBS_PARS_FRM_RECEIVED = 2
RBS_PARS_FRM_STARTED = 3
RBS_PARS_SN_RECEIVED = 4
RBS_PARS_LBL_RECEIVED = 5
RBS_PARS_TYPE_AT_RECEIVED = 6
RBS_PARS_TYPE_GET_RECEIVED =  7
RBS_PARS_TYPE_SET_RECEIVED = 8
RBS_PARS_ARG_BYTE_RECEIVED = 9

// RX FSM states
RBS_FSM_RX_ACCEPT_FRAME = 0
RBS_FSM_RX_END = 1
RBS_FSM_RX_DISCARD_FRAME = 2
RBS_FSM_RX_RST_RECEIVED = 3
RBS_FSM_RX_SEND_AKN_0 = 4
RBS_FSM_RX_SEND_AKN_1 = 5
RBS_FSM_RX_START = 6
RBS_FSM_RX_WAIT_FRAME_0 = 7
RBS_FSM_RX_WAIT_FRAME_1 = 8

// TX FSM states
RBS_FSM_TX_CONFIRMED = 0
RBS_FSM_TX_FAIL = 1
RBS_FSM_TX_RST_RECEIVED = 2
RBS_FSM_TX_SEND_FRAME_0 = 3
RBS_FSM_TX_SEND_FRAME_1 = 4
RBS_FSM_TX_START = 5
RBS_FSM_TX_WAIT_AKN_0 = 6
RBS_FSM_TX_WAIT_AKN_1 = 7

// Parser related variables
RBS_parser_state = RBS_PARS_FRM_NOT_STARTED     // Parser FSM initial state
RBS_frame_received = false      // does  packet received
RBS_incoming_byte = ""          // last receivet byte
RBS_parsed_sn = ""              // parsed sequence number
RBS_parsed_lbl = ""            // parsed command label
RBS_parsed_type = ""           // parsed command type
RBS_parsed_arg_str = ""        // parsed command argument
RBS_frame_str = ""             // pointer to store Packet (every time allocated/deallocated before/after sending)
RBS_curr_arg_idx = 0            // counter to store index of element in command argument array to save incoming byte

// Pre-defined handshake frames
RBS_predef_frame_akn0 = "[0#@+]"
RBS_predef_frame_akn1 = "[1#@+]"
RBS_predef_frame_nak = "[0#@-]"
RBS_predef_frame_rdy = "[0#@>]"
RBS_predef_frame_rst = "[0#@!]"

// Receiver related
RBS_rx_state = RBS_FSM_RX_START                    // Receiver FSM state
RBS_accept_frame = false                       // is received frame accepted
RBS_expected_rx_frame_sn = RBS_SN_ZERO         // expected sequence number of received frame
RBS_rx_not_received_cnt = 0

// Transmitter related
RBS_tx_state = RBS_FSM_TX_START         // Transmitter FSM state
RBS_tx_confirmed = false             // is transmission confirmed
RBS_tx_frame_sn = RBS_SN_ZERO               // Sequence number of a Frame to transmit
RBS_tx_att_cnt = 0                 // counter for transmission attempts

RBS_wait_tmr = 0     // Timer to measure waiting for akn time

RBS_RAW_OUT = ""            // stores last raw data sent out by UART
RBS_RAW_IN = ""             // stores last raw data received by UART

Return
//==============================================================================================
Sub RBS_receiveFrame()
    
    // disable interrupts
    //AllowEvents OFF
    
    //Call LOG_write("FSTART: RBS_receivePacket()") 
    
    //2026-10-10
    //ClearSerBuffer 1   
    
    delay _RBS_SER_TIMEOUT
    
    run_fsm = true
         
    // run parser FSM 
    _RBS_parser_state = _RBS_PARS_FRM_NOT_STARTED
    
    
    While run_fsm == true
        
        // -----------------------
        // STATE "PK_NOT_RECEIVED"    
        If _RBS_parser_state == _RBS_PARS_FRM_NOT_RECEIVED
            _RBS_frame_received = false
            run_fsm = false 
        // END STATE "PK_NOT_RECEIVED" 


        // ----------------------
        // STATE "PK_NOT_STARTED"
        ElseIf _RBS_parser_state == _RBS_PARS_FRM_NOT_STARTED
            
            // clear all
            Call RBS_resetParser()
            
            CheckSerBuffer num_of_bytes
            if num_of_bytes > 0
               SerBytesIn 1, incoming_byte, vn_ActualNumberRead
               _RBS_incoming_byte = incoming_byte
               _RBS_RAW_IN = ToString(_RBS_RAW_IN) + ToString(_RBS_incoming_byte)
                    
                if _RBS_incoming_byte == _RBS_FRM_FST
                    _RBS_parser_state = _RBS_PARS_FRM_STARTED
                else
                    _RBS_parser_state = _RBS_PARS_FRM_NOT_STARTED
                endif
            else
                _RBS_parser_state = _RBS_PARS_FRM_NOT_RECEIVED
            endif          
        // END STATE "PK_NOT_STARTED" 


        // -------------------
        // STATE "PK_RECEIVED"
        ElseIf _RBS_parser_state == _RBS_PARS_FRM_RECEIVED 
            _RBS_frame_received = true
            run_fsm = false 
             Call LOG_write("RBS_receivePacket(): Packet received! " + ToString(_RBS_FRM_FST) + ToString(_RBS_parsed_sn) + ToString(_RBS_parsed_lbl) + ToString(_RBS_parsed_type) + ToString(_RBS_parsed_arg_str) + ToString(_RBS_FRM_LST))      
        // END STATE "PK_RECEIVED"
        
        
        // ------------------
        // STATE "FRM_STARTED"
        ElseIf _RBS_parser_state == _RBS_PARS_FRM_STARTED

            CheckSerBuffer num_of_bytes
            if num_of_bytes > 0
               SerBytesIn 1, incoming_byte, vn_ActualNumberRead
               _RBS_incoming_byte = incoming_byte
               _RBS_RAW_IN = ToString(_RBS_RAW_IN) + ToString(_RBS_incoming_byte)
                               
                if _RBS_incoming_byte == _RBS_SN_ZERO || _RBS_incoming_byte == _RBS_SN_ONE
                    _RBS_parsed_sn = _RBS_incoming_byte
                    _RBS_parser_state = _RBS_PARS_SN_RECEIVED
                else            
                    _RBS_parser_state = _RBS_PARS_FRM_NOT_STARTED
                endif
            else
                _RBS_parser_state = _RBS_PARS_FRM_NOT_RECEIVED
            endif
        // END STATE "PK_STARTED"
        
        
        // ------------------
        // STATE "SN_RECEIVED"        
        Elseif _RBS_parser_state == _RBS_PARS_SN_RECEIVED
            
            CheckSerBuffer num_of_bytes
            if num_of_bytes > 0
               SerBytesIn 1, incoming_byte, vn_ActualNumberRead
               _RBS_incoming_byte = incoming_byte
               _RBS_RAW_IN = ToString(_RBS_RAW_IN) + ToString(_RBS_incoming_byte)
               
               if _RBS_incoming_byte == _RBS_FRM_FST || _RBS_incoming_byte == _RBS_FRM_FST
                  _RBS_parser_state = _RBS_PARS_FRM_NOT_STARTED
               else
                  _RBS_parsed_lbl = _RBS_incoming_byte
                  _RBS_parser_state = _RBS_PARS_LBL_RECEIVED
               endif
               
            else
                _RBS_parser_state = _RBS_PARS_FRM_NOT_RECEIVED
            endif                
        // END STATE "SN_RECEIVED"  
        
        
        // --------------------
        // STATE "LBL_RECEIVED"
        ElseIf _RBS_parser_state == _RBS_PARS_LBL_RECEIVED
        
            CheckSerBuffer num_of_bytes
            if num_of_bytes > 0
               SerBytesIn 1, incoming_byte, vn_ActualNumberRead
               _RBS_incoming_byte = incoming_byte
               _RBS_RAW_IN = ToString(_RBS_RAW_IN) + ToString(_RBS_incoming_byte)
                
                if _RBS_incoming_byte == _RBS_TYPE_GET
                    _RBS_parsed_type = _RBS_TYPE_GET
                    _RBS_parser_state = _RBS_PARS_TYPE_GET_RECEIVED
                elseif _RBS_incoming_byte == _RBS_TYPE_SET
                    _RBS_parsed_type = _RBS_TYPE_SET
                   _RBS_parser_state = _RBS_PARS_TYPE_SET_RECEIVED
                elseif _RBS_incoming_byte == _RBS_TYPE_AT
                    _RBS_parsed_type = _RBS_TYPE_AT
                    _RBS_parser_state = _RBS_PARS_TYPE_AT_RECEIVED                
                else
                    _RBS_parser_state = _RBS_PARS_FRM_NOT_STARTED
                endif
            else
                _RBS_parser_state = _RBS_PARS_FRM_NOT_RECEIVED
            endif
        // END STATE "LBL_RECEIVED"
        
        
        // ------------------------
        // STATE "TYPE_AT_RECEIVED"
        ElseIf _RBS_parser_state == _RBS_PARS_TYPE_AT_RECEIVED
            
            CheckSerBuffer num_of_bytes
            if num_of_bytes > 0
               SerBytesIn 1, incoming_byte, vn_ActualNumberRead
               _RBS_incoming_byte = incoming_byte
               _RBS_RAW_IN = ToString(_RBS_RAW_IN) + ToString(_RBS_incoming_byte)
                
                if _RBS_incoming_byte == _RBS_FRM_FST || _RBS_incoming_byte == _RBS_FRM_LST
                    _RBS_parser_state = _RBS_PARS_FRM_NOT_STARTED
                else
                    _RBS_parsed_arg_str += _RBS_incoming_byte
                    _RBS_curr_arg_idx++
                    _RBS_parser_state = _RBS_PARS_ARG_BYTE_RECEIVED
                endif            
            else
                _RBS_parser_state = _RBS_PARS_FRM_NOT_RECEIVED
            endif
        // END STATE "TYPE_AT_RECEIVED" 
        
        
        // STATE "TYPE_SET_RECEIVED"
        ElseIf _RBS_parser_state == _RBS_PARS_TYPE_SET_RECEIVED
            
            CheckSerBuffer num_of_bytes
            if num_of_bytes > 0
               SerBytesIn 1, incoming_byte, vn_ActualNumberRead
               _RBS_incoming_byte = incoming_byte
               _RBS_RAW_IN = ToString(_RBS_RAW_IN) + ToString(_RBS_incoming_byte)
                
                if _RBS_incoming_byte == _RBS_FRM_FST || _RBS_incoming_byte == _RBS_FRM_LST
                    _RBS_parser_state = _RBS_PARS_FRM_NOT_STARTED
                else
                    _RBS_parsed_arg_str += _RBS_incoming_byte
                    _RBS_curr_arg_idx++
                    _RBS_parser_state = _RBS_PARS_ARG_BYTE_RECEIVED
                endif  
            else
                _RBS_parser_state = _RBS_PARS_FRM_NOT_RECEIVED
            endif           
        // END STATE "TYPE_SET_RECEIVED"
        
            
        // -------------------------            
        // STATE "TYPE_GET_RECEIVED"
        ElseIf _RBS_parser_state == _RBS_PARS_TYPE_GET_RECEIVED
        
            CheckSerBuffer num_of_bytes
            if num_of_bytes > 0
               SerBytesIn 1, incoming_byte, vn_ActualNumberRead
               _RBS_incoming_byte = incoming_byte
               _RBS_RAW_IN = ToString(_RBS_RAW_IN) + ToString(_RBS_incoming_byte)
                
                if _RBS_incoming_byte == _RBS_FRM_LST
                    _RBS_parser_state = _RBS_PARS_FRM_RECEIVED
                else
                    _RBS_parser_state = _RBS_PARS_FRM_NOT_STARTED
                endif
            else
                _RBS_parser_state = _RBS_PARS_FRM_NOT_RECEIVED
            endif    
        // END STATE "TYPE_GET_RECEIVED"                                 
        
        // -------------------------
        // STATE "ARG_BYTE_RECEIVED"
        ElseIf _RBS_parser_state == _RBS_PARS_ARG_BYTE_RECEIVED
            
            CheckSerBuffer num_of_bytes
            if num_of_bytes > 0
                          
                if _RBS_curr_arg_idx > _RBS_ARG_LMAX
                    _RBS_parser_state = _RBS_PARS_FRM_NOT_STARTED
                else
                    SerBytesIn 1, incoming_byte, vn_ActualNumberRead
                    _RBS_incoming_byte = incoming_byte
                    _RBS_RAW_IN = ToString(_RBS_RAW_IN) + ToString(_RBS_incoming_byte)
                    
                    if _RBS_incoming_byte == _RBS_FRM_LST
                        _RBS_parser_state = _RBS_PARS_FRM_RECEIVED
                    else
                        _RBS_parsed_arg_str = _RBS_parsed_arg_str + _RBS_incoming_byte
                        _RBS_curr_arg_idx++
                        _RBS_parser_state = _RBS_PARS_ARG_BYTE_RECEIVED
                    endif
               endif               
            else
                _RBS_parser_state = _RBS_PARS_FRM_NOT_RECEIVED
            endif
        // END STATE "ARG_BYTE_RECEIVED"
        
        
        // DEFAULT/ UNDEFINED STATE
        Else
            _RBS_parser_state = _RBS_PARS_FRM_NOT_STARTED
        Endif
    
    Wend
    
Return _RBS_frame_received


//==============================================================================================
Sub RBS_receiveARQ()

    //Call LOG_write("RBS_receiveARQ(): Start")

    //_RBS_accept_frame = false
    run_fsm = true
    _RBS_rx_state = _RBS_FSM_RX_START
    
    While run_fsm == true
    
          // State RX_ACCEPT_FRAME
          if _RBS_rx_state == _RBS_FSM_RX_ACCEPT_FRAME
             //Call LOG_write("RBS_receiveARQ(): FSM state RX_ACCEPT_FRAME")
             
             _RBS_accept_frame = true
             
             // Next state
             if _RBS_parsed_sn == _RBS_SN_ZERO
                _RBS_rx_state = _RBS_FSM_RX_SEND_AKN_1
                //Call LOG_write("RBS_receiveARQ(): FSM next state RX_SEND_AKN_1")
             elseif _RBS_parsed_sn == _RBS_SN_ONE
                _RBS_rx_state = _RBS_FSM_RX_SEND_AKN_0
                //Call LOG_write("RBS_receiveARQ(): FSM next state RX_SEND_AKN_0")
             else
                _RBS_rx_state = _RBS_FSM_RX_DISCARD_FRAME
                //Call LOG_write("RBS_receiveARQ(): FSM next state RX_DISCARD_FRAME")
             endif
             
             //Call LOG_write("RBS_receiveARQ(): FSM state RX_ACCEPT_FRAME end")  
          // End State RX_ACCEPT_FRAME
          
          
          // State RX_END
          elseif  _RBS_rx_state == _RBS_FSM_RX_END
             //Call LOG_write("RBS_receiveARQ(): FSM state RX_END")
             run_fsm = false
             //Call LOG_write("RBS_receiveARQ()): FSM state RX_END end")
          // End State RX_END
          

          // State RX_DISCARD_FRAME
          elseif _RBS_rx_state == _RBS_FSM_RX_DISCARD_FRAME
             //Call LOG_write("RBS_receiveARQ(): FSM state RX_DISCARD_FRAME")
             
             _RBS_accept_frame = false
                     
             // Next state
             _RBS_rx_state = _RBS_FSM_RX_END
          // End State RX_DISCARD_FRAME
          
          
          // State RX_RST RECEIVED
          elseif _RBS_rx_state == _RBS_FSM_RX_RST_RECEIVED 
          
             //Call LOG_write("RBS_receiveARQ(): FSM state RX_RST RECEIVED")
             Call RBS_resetRX()
             Call RBS_resetTX()
             
             //2026-08-01
             //_RBS_accept_frame = true
             //_RBS_rx_state = _RBS_FSM_RX_END 
             
             // ORIGINAL
             //_RBS_rx_state = _RBS_FSM_RX_DISCARD_FRAME 
             
             // 2026-10-07
             _RBS_rx_state = _RBS_FSM_RX_START
                                                 
          // End State RX_RST RECEIVED       
          
          
          // State RX_SEND_AKN_0
          elseif _RBS_rx_state == _RBS_FSM_RX_SEND_AKN_0
            // Call LOG_write("RBS_receiveARQ(): FSM state RX_SEND_AKN_0")
             Call RBS_sendFrame(_RBS_predef_frame_akn0)
             
             if _RBS_DOUBLE_AKN == true
                Call RBS_sendFrame(_RBS_predef_frame_akn0)
             endif

             _RBS_expected_rx_frame_sn = _RBS_SN_ZERO
             
             // Next state
             _RBS_rx_state = _RBS_FSM_RX_END
          // EndState RX_SEND_AKN_0
          
          
          // State RX_SEND_AKN_1
          elseif _RBS_rx_state == _RBS_FSM_RX_SEND_AKN_1
             //Call LOG_write("RBS_receiveARQ(): FSM state RX_SEND_AKN_1")
             Call RBS_sendFrame(_RBS_predef_frame_akn1)
             
             if _RBS_DOUBLE_AKN == true
                Call RBS_sendFrame(_RBS_predef_frame_akn1)
             endif
             
             _RBS_expected_rx_frame_sn = _RBS_SN_ONE
             
             // Next state
             _RBS_rx_state = _RBS_FSM_RX_END
          // EndState RX_SEND_AKN_1
          
          
          // State RX_START
          elseif _RBS_rx_state == _RBS_FSM_RX_START
             //Call LOG_write("RBS_receiveARQ(): FSM state RX_START")
             
             //2026-10-07
             //ClearSerBuffer 1
         
             _RBS_accept_frame = false
             
             // Next state
             if _RBS_expected_rx_frame_sn == _RBS_SN_ZERO
                _RBS_rx_state = _RBS_FSM_RX_WAIT_FRAME_0
             elseif _RBS_expected_rx_frame_sn == _RBS_SN_ONE
                _RBS_rx_state = _RBS_FSM_RX_WAIT_FRAME_1
             else
                _RBS_rx_state = _RBS_FSM_RX_DISCARD_FRAME
             endif                
          // EndState RX_START

                             
          // State RX_RX_WAIT_FRAME_0
          elseif _RBS_rx_state == _RBS_FSM_RX_WAIT_FRAME_0
             //Call LOG_write("RBS_receiveARQ(): FSM state RX_WAIT_FRAME_0")
             
             _RBS_wait_tmr = Timer()
             
             While(Timer() - _RBS_wait_tmr <= _RBS_RESP_WAIT_TIME)
             
                 // Call RBS_resetParser()
                 Call RBS_receiveFrame()
                 
                 Call RBS_checkRST()
                 
                 // Next state
                 if (_RBS_frame_received == true) && (RBS_checkRST__Result == true)
                   _RBS_rx_state = _RBS_FSM_RX_RST_RECEIVED
                   Break
                 endif
             
                 if (_RBS_frame_received == true) && (_RBS_parsed_lbl != _RBS_LBL_HDS) && (_RBS_parsed_sn == _RBS_SN_ZERO)
                    _RBS_rx_state = _RBS_FSM_RX_ACCEPT_FRAME
                    Break
                 else
                    _RBS_rx_state = _RBS_FSM_RX_DISCARD_FRAME
                 endif
             Wend
             
             //Call LOG_write("RBS_receiveARQ(): FSM state RX_WAIT_FRAME_0 end")
          // EndState RX_RX_WAIT_FRAME_0 
          
                 
          // State RX_RX_WAIT_FRAME_1
          elseif _RBS_rx_state == _RBS_FSM_RX_WAIT_FRAME_1
            // Call LOG_write("RBS_receiveARQ(): FSM state RX_WAIT_FRAME_1")
             
             _RBS_wait_tmr = Timer()
             
             While(Timer() - _RBS_wait_tmr <= _RBS_RESP_WAIT_TIME)             
             
                 //Call RBS_resetParser()
                 Call RBS_receiveFrame()
                 
                 Call RBS_checkRST()
                 
                 // Next state
                 if (_RBS_frame_received == true) && (RBS_checkRST__Result == true)
                   _RBS_rx_state = _RBS_FSM_RX_RST_RECEIVED
                   Break
                 endif
             
                 if (_RBS_frame_received == true) && (_RBS_parsed_lbl != _RBS_LBL_HDS) && (_RBS_parsed_sn == _RBS_SN_ONE)
                    _RBS_rx_state = _RBS_FSM_RX_ACCEPT_FRAME
                    Break
                 else
                    _RBS_rx_state = _RBS_FSM_RX_DISCARD_FRAME                   
                 endif
             Wend
          // EndState RX_RX_WAIT_FRAME_1
          
           
          // Default state               
          else
              //Call LOG_write("RBS_receiveARQ(): FSM state Default state start")
              _RBS_rx_state = _RBS_FSM_RX_DISCARD_FRAME
              //Call LOG_write("RBS_receiveARQ(): FSM next state RX_DISCARD_FRAME")
              //Call LOG_write("RBS_receiveARQ(): FSM state Default state end")
          // End Default state           
          endif                       
    Wend
    //Call LOG_write("RBS_receiveARQ(): End, Return = " + ToString(_RBS_accept_frame))
Return _RBS_accept_frame


//==============================================================================================
Sub RBS_transmitARQ()

   // Call LOG_write("RBS_transmitARQ(): Start")
   
    _RBS_tx_state = _RBS_FSM_TX_START
    
    run_fsm = true
    
    While run_fsm == true

          // ------------------
          // State TX_CONFIRMED
          if _RBS_tx_state == _RBS_FSM_TX_CONFIRMED
             //Call LOG_write("RBS_transmitARQ(): FSM state TX_CONFIRMED")
                       
             _RBS_frame_str = ""
             _RBS_tx_confirmed = true
             
             if _RBS_tx_frame_sn == _RBS_SN_ZERO
                _RBS_tx_frame_sn = _RBS_SN_ONE
             else
                _RBS_tx_frame_sn = _RBS_SN_ZERO
             endif
             
             run_fsm = false          
          // End State TX_CONFIRMED
          
          
          // -------------
          // State TX_FAIL
          elseif _RBS_tx_state == _RBS_FSM_TX_FAIL
            // Call LOG_write("RBS_transmitARQ(): FSM state TX_FAIL")
             
             _RBS_frame_str = ""
             _RBS_tx_confirmed = false   
                          
             run_fsm = false        
          // End State TX_FAIL

                    
          // State TX_RST RECEIVED
          elseif _RBS_tx_state == _RBS_FSM_TX_RST_RECEIVED 
          
            // Call LOG_write("RBS_transmitARQ(): FSM state TX_RST RECEIVED")
             Call RBS_resetRX()
             Call RBS_resetTX()
             
             // ORIGINAL
             //_RBS_tx_state = _RBS_FSM_TX_FAIL 
             
             // 2026-10-07
             _RBS_tx_state = _RBS_FSM_TX_START
             
             //2026-08-02
             //_RBS_tx_frame_sn = _RBS_SN_ZERO
             //_RBS_tx_state = _RBS_FSM_TX_START                                      
          // End State TX_RST RECEIVED  
                    
          
          // State TX_SEND_FRAME_0
          elseif _RBS_tx_state == _RBS_FSM_TX_SEND_FRAME_0
            // Call LOG_write("RBS_transmitARQ(): FSM state TX_SEND_FRAME_0")
          
             if _RBS_tx_att_cnt <= _RBS_ARQ_ATT_MAX

                _RBS_frame_str = Replace(_RBS_frame_str, _RBS_SN_ZERO, 2)
                Call RBS_sendFrame(_RBS_frame_str)         
                _RBS_tx_att_cnt++
                _RBS_tx_state = _RBS_FSM_TX_WAIT_AKN_1
             else
                _RBS_tx_state = _RBS_FSM_TX_FAIL 
             endif                    
          // End State TX_SEND_FRAME_0

           
          // State TX_SEND_FRAME_1                   
          elseif _RBS_tx_state == _RBS_FSM_TX_SEND_FRAME_1
            // Call LOG_write("RBS_transmitARQ(): FSM state TX_SEND_FRAME_1")
          
             if _RBS_tx_att_cnt <= _RBS_ARQ_ATT_MAX

                _RBS_frame_str = Replace(_RBS_frame_str, _RBS_SN_ONE, 2)
                Call RBS_sendFrame(_RBS_frame_str)
                _RBS_tx_att_cnt++
                _RBS_tx_state = _RBS_FSM_TX_WAIT_AKN_0
             else
                _RBS_tx_state = _RBS_FSM_TX_FAIL 
             endif                      
          // End State TX_SEND_FRAME_1
          
          
          // State TX_START
          elseif _RBS_tx_state == _RBS_FSM_TX_START
            // Call LOG_write("RBS_transmitARQ(): FSM state TX_START")
             
             _RBS_parsed_sn = _RBS_SN_ZERO // default, will be changed later
             Call RBS_makeFrame() 
             _RBS_tx_confirmed = false
             _RBS_tx_att_cnt = 0
             
             if _RBS_tx_frame_sn == _RBS_SN_ZERO
                _RBS_tx_state = _RBS_FSM_TX_SEND_FRAME_0
             elseif _RBS_tx_frame_sn == _RBS_SN_ONE
                _RBS_tx_state = _RBS_FSM_TX_SEND_FRAME_1
             else
                _RBS_tx_state = _RBS_FSM_TX_FAIL 
             endif  
                           
          // End State TX_START
          
          
          // State TX_WAIT_AKN_0
          elseif _RBS_tx_state == _RBS_FSM_TX_WAIT_AKN_0
            // Call LOG_write("RBS_transmitARQ(): FSM state TX_WAIT_AKN_0")
          
             _RBS_wait_tmr = Timer()
             
             While(Timer() - _RBS_wait_tmr <= _RBS_RESP_WAIT_TIME)
             
                 Call RBS_receiveFrame()
                 
                 if _RBS_frame_received == true
                 
                    Call RBS_checkRST()
                 
                    if RBS_checkRST__Result == true
                       _RBS_tx_state = _RBS_FSM_TX_RST_RECEIVED
                       Break
                    endif                 
                 
                    if (_RBS_frame_received == true) && (_RBS_parsed_lbl == _RBS_LBL_HDS) && (_RBS_parsed_arg_str == _RBS_ARG_AKN) && (_RBS_parsed_sn == _RBS_SN_ZERO)
                       _RBS_tx_state = _RBS_FSM_TX_CONFIRMED
                       Break 
                    else    
                       _RBS_tx_state = _RBS_FSM_TX_SEND_FRAME_1
                    endif
                 else
                 endif            
             Wend                    
          // End State TX_WAIT_AKN_0
          
          
          // State TX_WAIT_AKN_1
          elseif _RBS_tx_state == _RBS_FSM_TX_WAIT_AKN_1
            // Call LOG_write("RBS_transmitARQ(): FSM state TX_WAIT_AKN_1")
              
             _RBS_wait_tmr = Timer()
             
             While(Timer() - _RBS_wait_tmr <= _RBS_RESP_WAIT_TIME)
             
                 Call RBS_receiveFrame()
                 
                 if _RBS_frame_received == true
                    
                    Call RBS_checkRST()
                 
                    if RBS_checkRST__Result == true
                       _RBS_tx_state = _RBS_FSM_TX_RST_RECEIVED
                       Break
                    endif                   
                                  
                    if (_RBS_frame_received == true) && (_RBS_parsed_lbl == _RBS_LBL_HDS) && (_RBS_parsed_arg_str == _RBS_ARG_AKN) && (_RBS_parsed_sn == _RBS_SN_ONE)
                       _RBS_tx_state = _RBS_FSM_TX_CONFIRMED
                       Break
                    else            
                       _RBS_tx_state = _RBS_FSM_TX_SEND_FRAME_0
                    endif
                 else
                 endif 
             Wend                        
          // End State TX_WAIT_AKN_1
          
          
          // State Default
          else
             // Call LOG_write("RBS_transmitARQ(): FSM state DEFAULT")
              _RBS_tx_state = _RBS_FSM_TX_FAIL                     
          // EndState Default
          endif
    
    Wend

   // Call LOG_write("RBS_transmitARQ(): End, Return = " + ToString(_RBS_tx_confirmed))

Return  _RBS_tx_confirmed

//==============================================================================================
Sub RBS_commInquiryData(dataLbl)

  // Fill frame fields with request for sensor data
  _RBS_parsed_lbl = dataLbl                     // label of data to be inquiried
  _RBS_parsed_type = _RBS_TYPE_GET              // "get" type message
  
  received = false

  // try to communicate few times
  // High level comm protocol
  For i = 0 to _RBS_COMM_TX_ATT_MAX

    Call RBS_transmitARQ()

    // if request has been tranmitted
    if RBS_transmitARQ__Result == true
    
      Call RBS_receiveARQ()
      received = RBS_receiveARQ__Result // receive reply

      // if reply has been received
      if received == true
        break
      endif
    else
      // Reset comm on both sides
      Call RBS_resetComm()    
    endif    
  Next  

Return received

//==============================================================================================
Sub RBS_commSendParsedFrame()

  // For master: Send Frame with set-type command, ARQ Stop&Wait + reset both parties RX/TX on error

  transmitted = false;
  
  For i = 0 to _RBS_COMM_TX_ATT_MAX

    Call RBS_transmitARQ()
    transmitted = RBS_transmitARQ__Result
    
    if transmitted == true
      break
    else
        // Reset comm on both sides
        Call RBS_resetComm()   
    endif
  Next

Return transmitted


//==============================================================================================
Sub RBS_commSendCommand(dataLbl, dataVal)

  // Fill frame fields with stop command
  _RBS_parsed_lbl = dataLbl
  _RBS_parsed_type = _RBS_TYPE_SET
  _RBS_parsed_arg_str = dataVal
  
  Call RBS_commSendParsedFrame()
  
Return RBS_commSendParsedFrame__Result


//==============================================================================================
Sub RBS_commReceive()

  // For master/slave: Receive frame, reset both parties RX/TX if long time no incoming frames 

  received = false

  if _RBS_rx_not_received_cnt > _RBS_COMM_RX_ATT_MAX

    _RBS_rx_not_received_cnt = 0    // Reset counter    
    Call RBS_resetComm()            // Reset comm on both sides     
  endif

  Call RBS_receiveARQ()
  received = RBS_receiveARQ__Result

  if received == true
    _RBS_rx_not_received_cnt = 0
  else
    _RBS_rx_not_received_cnt++
  endif
 
Return received

//==============================================================================================
Sub RBS_makeFrame()

    if _RBS_parsed_type == _RBS_TYPE_GET
        _RBS_frame_str = _RBS_FRM_FST + _RBS_parsed_sn + _RBS_parsed_lbl + _RBS_parsed_type + _RBS_FRM_LST
    else
        _RBS_frame_str = _RBS_FRM_FST + _RBS_parsed_sn + _RBS_parsed_lbl + _RBS_parsed_type + _RBS_parsed_arg_str + _RBS_FRM_LST
    endif
Return


//==============================================================================================
Sub RBS_sendFrame(str_to_send)
   
    _RBS_RAW_OUT = str_to_send
    Call LOG_write("RBS_sendPacket(): Sending frame " + ToString(_RBS_RAW_OUT))
    SerOut _RBS_RAW_OUT  
    
Return

//==============================================================================================
Sub RBS_sendParsedFrame()

    Call RBS_makeFrame()
    _RBS_RAW_OUT = _RBS_frame_str 
    Call LOG_write("RBS_sendPacket(): Sending frame " + ToString(_RBS_RAW_OUT))
    SerOut _RBS_RAW_OUT 
    _RBS_frame_str = ""      
    
Return


//==============================================================================================
Sub RBS_resetParser()

    _RBS_frame_received = false     // does  packet received
    _RBS_curr_arg_idx = 1           // restart argument string characters counter
    _RBS_parsed_sn = ""
    _RBS_parsed_lbl = ""            // parsed command label
    _RBS_parsed_type = ""           // parsed command type
    _RBS_parsed_arg_str = ""        // parsed command argument
    
Return


//==============================================================================================
Sub RBS_resetRX()

    // Receiver related
    //_RBS_rx_state = _RBS_FSM_RX_DISCARD_FRAME                    // Receiver FSM state
    _RBS_accept_frame = false                       // is received frame accepted
    _RBS_expected_rx_frame_sn = _RBS_SN_ZERO         // expected sequence number of received frame
    
    // Try 2026-07-31
    //Call RBS_receiveFrame()
    //Call RBS_sendFrame(_RBS_predef_frame_akn0)
    //Call RBS_sendFrame(_RBS_predef_frame_akn0)

Return


//==============================================================================================
Sub RBS_resetTX()

    // Transmitter related
    //_RBS_tx_state = _RBS_FSM_TX_FAIL         // Transmitter FSM state
    _RBS_tx_confirmed = false             // is transmission confirmed
    _RBS_tx_frame_sn = _RBS_SN_ZERO               // Sequence number of a Frame to transmit
    _RBS_tx_att_cnt = 0                 // counter for transmission attempts   
    
    // Try 2026-07-31
    // Update outcoming frame
    //_RBS_parsed_sn = _RBS_SN_ZERO
    //Call RBS_makeFrame()
    //Call RBS_sendParsedFrame()
    //Call RBS_sendFrame(_RBS_predef_frame_akn0)
    //Call RBS_sendFrame(_RBS_predef_frame_akn0) 
    
Return

    
//==============================================================================================            
Sub RBS_sendRST()

    _RBS_RAW_OUT = _RBS_predef_frame_rst 
    Call LOG_write("RBS_sendPacket(): Sending frame " + ToString(_RBS_RAW_OUT))
    SerOut _RBS_RAW_OUT 
    SerOut _RBS_RAW_OUT 

Return


//==============================================================================================  
Sub RBS_checkRST()

    isRST = false

    if (_RBS_parsed_lbl == _RBS_LBL_HDS) && (_RBS_parsed_arg_str == _RBS_ARG_RST)
       isRST = true
    else
       isRST = false
    endif
    
Return isRST


//==============================================================================================  
Sub RBS_resetComm()

  // Try 2026-07-31
  //_RBS_tx_frame_sn = _RBS_SN_ZERO
  //_RBS_expected_rx_frame_sn = _RBS_SN_ZERO
  //_RBS_parsed_sn = _RBS_SN_ZERO

  // Reset comm on both sides
  Call RBS_sendRST()
  Call RBS_resetRX()
  Call RBS_resetTX() 

  //Call RBS_sendFrame(_RBS_predef_frame_akn0)
  //Call RBS_sendFrame(_RBS_predef_frame_akn1)
  //Call RBS_sendFrame(_RBS_predef_frame_akn0)
  //Call RBS_sendFrame(_RBS_predef_frame_akn1)
  
  delay 5*_RBS_SER_TIMEOUT    
     
Return  

//==============================================================================================  
Sub RBS_transmitRdy()

    Call RBS_sendFrame(_RBS_predef_frame_rdy)
Return
