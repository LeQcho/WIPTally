/*
Copyright 2012-2014 Kasper Skårhøj, SKAARHOJ K/S, kasper@skaarhoj.com

This file is part of the Blackmagic Design ATEM Client library for Arduino

The ATEM library is free software: you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by the 
Free Software Foundation, either version 3 of the License, or (at your 
option) any later version.

The ATEM library is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE. 
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with the ATEM library. If not, see http://www.gnu.org/licenses/.


IMPORTANT: If you want to use this library in your own projects and/or products,
please play a fair game and heed the license rules! See our web page for a Q&A so
you can keep a clear conscience: http://skaarhoj.com/about/licenses/


*/



#include "ATEMbase.h"



/**
 * Constructor
 */
ATEMbase::ATEMbase() : _localPort(0), _serialOutput(0),
  _localPacketIdCounter(0), _initPayloadSent(false), _initPayloadSentAtPacketId(0),
  _hasInitialized(false), _isConnected(false), _isRejected(false), _sessionID(0),
  _lastContact(0), _lastRemotePacketID(0), _returnPacketLength(0),
  _cmdLength(0), _cmdPointer(0), _cBundle(false), _cBBO(0), _ATEMmodel(0),
  neverConnected(true), waitingForIncoming(false) {}

/**
 * Setting up IP address for the switcher (and local port to send packets from)
 * Using local port here is deprecated. Rather let the library pick a random one
 */
void ATEMbase::begin(const IPAddress ip){
	begin(ip, random(50100,65300));
}
void ATEMbase::begin(const IPAddress ip, const uint16_t localPort){

	neverConnected = true;
	waitingForIncoming = false;

  // Close the old socket before Wi-Fi recovery; never shallow-copy WiFiUDP.
  _Udp.stop();
  _isConnected = _isRejected = _hasInitialized = false;
  _resetState();

	_switcherIP = ip;			// Set switcher IP address
	_localPort = localPort;		// Set default local port

	_lastContact = 0;
	_serialOutput = 0;
	
	resetCommandBundle();
}

/**
 * Initiating connection handshake to the ATEM switcher
 */
void ATEMbase::connect() {
	connect(false);
}

/**
 * Initiating connection handshake to the ATEM switcher
 * If useFixedPortNumber is true, the same port number will be used on subsequent connects, otherwise - and recommended - a new, random port number is used.
 */
void ATEMbase::connect(const boolean useFixedPortNumber) {
  neverConnected = false;
  waitingForIncoming = false;
  _gapPending = false;
  _lastRemotePacketID = 0;
  _lastProgress = millis();
  _lastResendRequest = 0;
  diagnostics.inCm = false;
  ++diagnostics.sessions;
  _resetState();
	_localPacketIdCounter = 0;		// Init localPacketIDCounter to 0;
	_initPayloadSent = false;		// Will be true after initial payload of data is delivered (regular 12-byte ping packages are transmitted.)
	_hasInitialized = false;		// Will be true after initial payload of data is resent and received well
	_isConnected = false;			// Will be true after the initial hello-package handshakes.
	_isRejected = false;			// Will be true if the connection was rejected during hello-package handshakes.
	_sessionID = 0x53AB;			// Temporary session ID - a new will be given back from ATEM.
	_lastContact = millis();  		// Setting this, because even though we haven't had contact, it constitutes an attempt that should be responded to at least
	_initializationPackets.reset();
	_initPayloadSentAtPacketId = ATEM_maxInitPackageCount;	// The max value it can be
	uint16_t portNumber = useFixedPortNumber ? _localPort : random(50100,65300);

  _Udp.stop();
  if (!_Udp.begin(portNumber)) {
    ++diagnostics.socketErrors;
    if (_serialOutput) Serial.println(F("ATEM: UDP socket open failed"));
    return;
  }		

		
	// Send connectString to ATEM:
	if (_serialOutput) 	{
  		Serial.print(F("Sending connect packet to ATEM switcher on IP "));
		Serial.print(_switcherIP);
		Serial.print(F(" from port "));
		Serial.println(portNumber);
	}

	
	_wipeCleanPacketBuffer();
	_createCommandHeader(ATEM_headerCmd_HelloPacket, 12+8);
	_packetBuffer[12] = 0x01;	// This seems to be what the client should send upon first request. 
	_packetBuffer[9] = 0x3a;	// This seems to be what the client should send upon first request. 
	
	_sendPacketBuffer(20);
}

/**
 * Keeps connection to the switcher alive
 * Therefore: Call this in the Arduino loop() function and make sure it gets call at least 2 times a second
 * Other recommendations might come up in the future.
 */
void ATEMbase::runLoop() {
	runLoop(0);
}
void ATEMbase::_drainPacket() {
  while (_Udp.available()) _Udp.read(_packetBuffer, ATEM_packetBufferLength);
}

void ATEMbase::_requestMissing(uint16_t packetId) {
  _wipeCleanPacketBuffer();
  _createCommandHeader(ATEM_headerCmd_RequestNextAfter, 12);
  _packetBuffer[6] = highByte(packetId);
  _packetBuffer[7] = lowByte(packetId);
  _packetBuffer[8] = 0x01;
  _sendPacketBuffer(12);
  _lastResendRequest = millis();
}

void ATEMbase::runLoop(uint16_t delayTime) {
  if (neverConnected) connect();
  const uint32_t entered = millis();
  do {
    // Bound work per pass, so a burst cannot starve ESP-NOW or the watchdog.
    for (uint8_t batch = 0; batch < 32; ++batch) {
      const int size = _Udp.parsePacket();
      if (size <= 0) break;
      if (_Udp.remoteIP() != _switcherIP || _Udp.remotePort() != 9910) {
        _drainPacket();
        continue;
      }
      if (size < 12 || _Udp.read(_packetBuffer, 12) != 12) {
        ++diagnostics.malformed;
        _drainPacket();
        continue;
      }
      const uint16_t length = word(_packetBuffer[0] & 7, _packetBuffer[1]);
      const uint8_t flags = _packetBuffer[0] >> 3;
      const uint16_t session = word(_packetBuffer[2], _packetBuffer[3]);
      const uint16_t id = word(_packetBuffer[10], _packetBuffer[11]);
      if (length != size || ((flags & ATEM_headerCmd_AckRequest) && id >= ATEM_maxPacketId)) {
        ++diagnostics.malformed;
        _drainPacket();
        continue;
      }
      ++diagnostics.received;
      if (flags & ATEM_headerCmd_HelloPacket) {
        if (length < 13 || _Udp.read(_packetBuffer, 1) != 1) {
          ++diagnostics.malformed;
          _drainPacket();
          continue;
        }
        const uint8_t kind = _packetBuffer[0];
        _drainPacket(); // ESP32 parsePacket will not advance past unread hello bytes.
        if (kind == 3) {
          _isRejected = true;
          _isConnected = _hasInitialized = false;
          _resetState();
          if (_serialOutput) Serial.println(F("ATEM: connection rejected (client limit)"));
          continue;
        }
        if (kind != 2) {
          if (_serialOutput) Serial.println(F("ATEM: restart requested during handshake"));
          connect();
          break;
        }
        // A repeated hello must be acknowledged without rewinding a live stream.
        if (!_isConnected) {
          _lastRemotePacketID = id;
          _lastProgress = millis();
        }
        _sessionID = session;
        _isConnected = true;
        _isRejected = false;
        _lastContact = millis();
        _wipeCleanPacketBuffer();
        _createCommandHeader(ATEM_headerCmd_Ack, 12, id);
        _packetBuffer[9] = 0x03;
        _sendPacketBuffer(12);
        continue;
      }
      if (!_isConnected || _isRejected) { _drainPacket(); continue; }
      // Some ATEM generations assign their final session ID in the first data packet.
      _sessionID = session;
      _lastContact = millis();
      if (flags & ATEM_headerCmd_RequestNextAfter) {
        // This tally client sends no state-changing commands; recover instead of
        // fabricating a retransmission for an unknown outbound command.
        _drainPacket();
        connect();
        break;
      }
      if (!(flags & ATEM_headerCmd_AckRequest)) { _drainPacket(); continue; }
      if (flags & ATEM_headerCmd_Resend) ++diagnostics.retransmits;
      const uint16_t distance = (id - _lastRemotePacketID + ATEM_maxPacketId) % ATEM_maxPacketId;
      if (distance == 1) {
        _lastRemotePacketID = id;
        _lastProgress = millis();
        _gapPending = false;
        if (length > 12) {
          ++diagnostics.payloads;
          _parsePacket(length);
        } else if (id > 1) {
          // Compatibility fallback for old mixers: all preceding IDs were
          // accepted in order, so a heartbeat safely ends their initial dump.
          _hasInitialized = true;
        }
      } else {
        _drainPacket();
        if (distance > 1 && distance < ATEM_maxPacketId / 2) {
          ++diagnostics.gaps;
          if (!_gapPending || uint32_t(millis() - _lastResendRequest) >= 50)
            _requestMissing((_lastRemotePacketID + 1) % ATEM_maxPacketId);
          _gapPending = true;
        }
        // Duplicates/older packets are re-ACKed but never applied to tally.
      }
      _wipeCleanPacketBuffer();
      _createCommandHeader(ATEM_headerCmd_Ack, 12, _lastRemotePacketID);
      _sendPacketBuffer(12);
    }
    if (_gapPending && uint32_t(millis() - _lastResendRequest) >= 100)
      _requestMissing((_lastRemotePacketID + 1) % ATEM_maxPacketId);
    if (hasTimedOut(_lastContact, 5000) ||
        (_gapPending && hasTimedOut(_lastProgress, 5000))) {
      if (_serialOutput) Serial.println(F("ATEM: timeout or stalled packet sequence; reconnecting"));
      connect();
    }
  } while (delayTime > 0 && !hasTimedOut(entered, delayTime));
}

/**
 * Returns last Remote Packet ID
 */
uint16_t ATEMbase::getATEM_lastRemotePacketId()	{
	return _lastRemotePacketID;
}

/**
 * Get ATEM session ID
 */
uint16_t ATEMbase::getSessionID() {
	return _sessionID;
}

/**
 * If true, we had a response from the switcher when trying to send a hello packet.
 */
bool ATEMbase::isConnected()	{
	return _isConnected;
}

/**
 * If true, the initial handshake and "stressful" information exchange has occured and now the switcher connection should be ready for operation. 
 */
bool ATEMbase::hasInitialized()	{
	return _hasInitialized;
}

/**
 * If true, the connection was rejected in the first responce, mening no empty spot.
 */
bool ATEMbase::isRejected() {
	return _isRejected;
}






/**************
 *
 * Buffer work
 *
 **************/

void ATEMbase::_createCommandHeader(const uint8_t headerCmd, const uint16_t lengthOfData)	{
	_createCommandHeader(headerCmd, lengthOfData, 0);
}
void ATEMbase::_createCommandHeader(const uint8_t headerCmd, const uint16_t lengthOfData, const uint16_t remotePacketID)	{

    _packetBuffer[0] = (headerCmd << 3) | (highByte(lengthOfData) & 0x07);  // Command bits + length MSB
    _packetBuffer[1] = lowByte(lengthOfData);  // length LSB

    _packetBuffer[2] = highByte(_sessionID);  // Session ID
    _packetBuffer[3] = lowByte(_sessionID);  // Session ID

    _packetBuffer[4] = highByte(remotePacketID);  // Remote Packet ID, MSB
    _packetBuffer[5] = lowByte(remotePacketID);  // Remote Packet ID, LSB
		
    if(!(headerCmd & (ATEM_headerCmd_HelloPacket | ATEM_headerCmd_Ack | ATEM_headerCmd_RequestNextAfter))) {
        _localPacketIdCounter = (_localPacketIdCounter + 1) % ATEM_maxPacketId;

//		if ((_localPacketIdCounter & 0xF) == 0xF) _localPacketIdCounter++;	// Uncommenting this line will jump the local package ID counter every 15 command - thereby introducing a stress test of the robustness of the "resent package" function from the ATEM switcher.

	    _packetBuffer[10] = highByte(_localPacketIdCounter);  // Local Packet ID, MSB
	    _packetBuffer[11] = lowByte(_localPacketIdCounter);  // Local Packet ID, LSB
    }
}
void ATEMbase::_sendPacketBuffer(uint8_t length)	{
	_Udp.beginPacket(_switcherIP,  9910);
	_Udp.write(_packetBuffer,length);
	_Udp.endPacket(); 	// TODO: Figure out why this may hang!!
}

/**
 * Sets all zeros in packet buffer:
 */
void ATEMbase::_wipeCleanPacketBuffer() {
	memset(_packetBuffer, 0, ATEM_packetBufferLength);
}

/**
 * Reads from UDP channel to buffer. Will fill the buffer to the max or to the size of the current segment being parsed
 * Returns false if there are no more bytes, otherwise true 
 */
bool ATEMbase::_readToPacketBuffer() {
	return _readToPacketBuffer(ATEM_packetBufferLength);
}
bool ATEMbase::_readToPacketBuffer(uint8_t maxBytes) {
	maxBytes = maxBytes<=ATEM_packetBufferLength ? maxBytes : ATEM_packetBufferLength;
	int remainingBytes = _cmdLength-8-_cmdPointer;

	if (remainingBytes>0)	{
		if (remainingBytes <= maxBytes)	{
			_Udp.read(_packetBuffer, remainingBytes);
			_cmdPointer+= remainingBytes;
			return false;	// Returns false if finished.
		} else {
			_Udp.read(_packetBuffer, maxBytes);
			_cmdPointer+= maxBytes;
			return true;	// Returns true if there are still bytes to be read.
		}
	} else {
		return false;
	}
}

/**
 * If a package longer than a normal acknowledgement is received from the ATEM Switcher we must read through the contents.
 * Usually such a package contains updated state information about the mixer
 * Selected information is extracted in this function and transferred to internal variables in this library.
 */
void ATEMbase::_parsePacket(uint16_t packetLength) {
  uint16_t offset = 12;
  while (offset < packetLength) {
    if (packetLength - offset < 8 || _Udp.read(_packetBuffer, 8) != 8) {
      ++diagnostics.commandErrors;
      _drainPacket();
      return;
    }
    _cmdLength = word(_packetBuffer[0], _packetBuffer[1]);
    _cmdPointer = 0;
    if (_cmdLength < 8 || _cmdLength > packetLength - offset) {
      ++diagnostics.commandErrors;
      _drainPacket();
      return;
    }
    char command[5];
    memcpy(command, _packetBuffer + 4, 4);
    command[4] = 0;
    if (strcmp(command, "InCm") == 0) {
      diagnostics.inCm = true;
      _hasInitialized = true;
    }
    if (_cmdLength > 8) _parseGetCommands(command);
    while (_readToPacketBuffer()) {}
    offset += _cmdLength;
  }
}

/**
 * This method should be overloaded in subclasses in order to handle specific get-commands
 */
void ATEMbase::_parseGetCommands(const char *cmdString)	{
//	uint8_t mE, keyer, mediaPlayer, aUXChannel, windowIndex, multiViewer, memory, colorGenerator, box;
//	uint16_t audioSource, videoSource;
//	long temp;
	
	uint8_t numberOfReads=1;
	while(_readToPacketBuffer())	{
		numberOfReads++;
	}

	#if ATEM_debug
 	if (_serialOutput & 0x80) {
		Serial.print(cmdString);
		Serial.print(", len: ");
		Serial.print(_cmdLength);
		Serial.print(", rds: ");
		Serial.println(numberOfReads);
	}
	#endif
}

void ATEMbase::_prepareCommandPacket(const char *cmdString, uint8_t cmdBytes, bool indexMatch)  {
	
		// First, in case of a command bundle, check if indexes are different OR if it's an entirely different command, then increase offset to accommodate new command:
		if (_cBundle)	{
			if (_returnPacketLength>0 && (!indexMatch || strncmp_P((char *)(_packetBuffer+12+_cBBO+4), cmdString, 4)))	{
				_cBBO = _returnPacketLength-12;
	  		}
		} else { 
			_wipeCleanPacketBuffer();	// For command bundles, this is already done...
		}

  	  _returnPacketLength = 12+_cBBO+(4+4+cmdBytes);
  
	  // Because we increased length of command, we need to check for buffer overflow:
  	  if (_returnPacketLength > ATEM_packetBufferLength)	{
  	  	  Serial.println(F("FATAL ERROR: Packet Buffer Overflow in the ATEM Library! Too long or too many commands bundled!\n HALT"));
  		  while(true){}	// STOP!
  	  }	

  		// Copy Command String:
		if (strlen_P(cmdString)==4)	{
			strncpy_P((char *)(_packetBuffer+12+_cBBO+4), cmdString, 4);
		} 
		#if ATEM_debug
		else Serial.println(F("Command Length > 4 ERROR"));
		#endif

	  // Command length:
	  _packetBuffer[12+_cBBO] = 0;	// MSB - but it's always under 256, so....
	  _packetBuffer[12+1+_cBBO] = 4+4+cmdBytes;	// LSB
}

void ATEMbase::_finishCommandPacket()	{
	if (!_cBundle)	{	

	  _createCommandHeader(ATEM_headerCmd_AckRequest, _returnPacketLength);
	  _sendPacketBuffer(_returnPacketLength);
	  _returnPacketLength = 0;

	} else {
    	  // Debugging info:
    /*	  for(uint8_t a=0; a<_returnPacketLength; a++)	{
  		  if (_packetBuffer[a]<16)	Serial.print("0");
    		  Serial.print(_packetBuffer[a], HEX);
    		  Serial.print(F("-"));
    	  }
    	  Serial.println();
		*/
	}
}



/**************
 *
 * Utilities from SkaarhojTools class:
 *
 **************/

/**
 * Setter method: If _serialOutput is set, the library may use Serial.print() to give away information about its operation - mostly for debugging.
 * 0= no output
 * 1= normal output (info)
 * 2= verbose
 * &0x80 (bit 7 set): verbose initial connection information
 */
void ATEMbase::serialOutput(uint8_t level) {
	_serialOutput = level;
}

/**
 * Timeout check
 */
bool ATEMbase::hasTimedOut(unsigned long time, unsigned long timeout) {
  return uint32_t(uint32_t(millis()) - uint32_t(time)) >= timeout;
}






uint8_t ATEMbase::getATEMmodel()	{
	return _ATEMmodel;
}






float ATEMbase::audioWord2Db(uint16_t input)  {  // -48 to +6 output
  // Formular: log10(input/128)*20-48;
  if (input<=32)  return -60;
  //return (log10(input)-2.1072099696)*20-48;
  // Better way?
  //return log10(input >> 5) * 20.0 - 60.0;

  return log10((float)input/(1<<11) / 16.0) * 20.0;
}
uint16_t ATEMbase::audioDb2Word(float input)  {  // -48 to +6 input
  //return (float)pow(10,(input+48)/20)*128;
	//return (uint16_t)pow(10, (input + 60.0) / 20.0) << 5;

	return pow(10, input/20.0) * 16.0 * (1<<11);
}









uint8_t ATEMbase::getVideoSrcIndex(uint16_t videoSrc)	{
	switch(videoSrc){
		case 0:  // Black
			return 0;
		case 1:  // Input 1
			return 1;
		case 2:  // Input 2
			return 2;
		case 3:  // Input 3
			return 3;
		case 4:  // Input 4
			return 4;
		case 5:  // Input 5
			return 5;
		case 6:  // Input 6
			return 6;
		case 7:  // Input 7
			return 7;
		case 8:  // Input 8
			return 8;
		case 9:  // Input 9
			return 9;
		case 10:  // Input 10
			return 10;
		case 11:  // Input 11
			return 11;
		case 12:  // Input 12
			return 12;
		case 13:  // Input 13
			return 13;
		case 14:  // Input 14
			return 14;
		case 15:  // Input 15
			return 15;
		case 16:  // Input 16
			return 16;
		case 17:  // Input 17
			return 17;
		case 18:  // Input 18
			return 18;
		case 19:  // Input 19
			return 19;
		case 20:  // Input 20
			return 20;
		case 1000:  // Color Bars
			return 21;
		case 2001:  // Color 1
			return 22;
		case 2002:  // Color 2
			return 23;
		case 3010:  // Media Player 1
			return 24;
		case 3011:  // Media Player 1 Key
			return 25;
		case 3020:  // Media Player 2
			return 26;
		case 3021:  // Media Player 2 Key
			return 27;
		case 4010:  // Key 1 Mask
			return 28;
		case 4020:  // Key 2 Mask
			return 29;
		case 4030:  // Key 3 Mask
			return 30;
		case 4040:  // Key 4 Mask
			return 31;
		case 5010:  // DSK 1 Mask
			return 32;
		case 5020:  // DSK 2 Mask
			return 33;
		case 6000:  // Super Source
			return 34;
		case 7001:  // Clean Feed 1
			return 35;
		case 7002:  // Clean Feed 2
			return 36;
		case 8001:  // Auxilary 1
			return 37;
		case 8002:  // Auxilary 2
			return 38;
		case 8003:  // Auxilary 3
			return 39;
		case 8004:  // Auxilary 4
			return 40;
		case 8005:  // Auxilary 5
			return 41;
		case 8006:  // Auxilary 6
			return 42;
		case 10010:  // ME 1 Prog
			return 43;
		case 10011:  // ME 1 Prev
			return 44;
		case 10020:  // ME 2 Prog
			return 45;
		case 10021:  // ME 2 Prev
			return 46;
		default:
			return 0;
	}
}
		
uint8_t ATEMbase::getAudioSrcIndex(uint16_t audioSrc)	{
	switch(audioSrc){
		case 1:  // Input 1
			return 0;
		case 2:  // Input 2
			return 1;
		case 3:  // Input 3
			return 2;
		case 4:  // Input 4
			return 3;
		case 5:  // Input 5
			return 4;
		case 6:  // Input 6
			return 5;
		case 7:  // Input 7
			return 6;
		case 8:  // Input 8
			return 7;
		case 9:  // Input 9
			return 8;
		case 10:  // Input 10
			return 9;
		case 11:  // Input 11
			return 10;
		case 12:  // Input 12
			return 11;
		case 13:  // Input 13
			return 12;
		case 14:  // Input 14
			return 13;
		case 15:  // Input 15
			return 14;
		case 16:  // Input 16
			return 15;
		case 17:  // Input 17
			return 16;
		case 18:  // Input 18
			return 17;
		case 19:  // Input 19
			return 18;
		case 20:  // Input 20
			return 19;
		case 1001:  // XLR
			return 20;
		case 1101:  // AES/EBU
			return 21;
		case 1201:  // RCA
			return 22;
		case 2001:  // MP1
			return 23;
		case 2002:  // MP2
			return 24;
		default:
			return 0;
	}
}

/*
 * Translating a index to a video source
 */
uint16_t ATEMbase::getVideoIndexSrc(uint8_t index)	{
  switch (index) {
    case 0:  // Black
      return 0;
    case 1:  // Input 1
      return 1;
    case 2:  // Input 2
      return 2;
    case 3:  // Input 3
      return 3;
    case 4:  // Input 4
      return 4;
    case 5:  // Input 5
      return 5;
    case 6:  // Input 6
      return 6;
    case 7:  // Input 7
      return 7;
    case 8:  // Input 8
      return 8;
    case 9:  // Input 9
      return 9;
    case 10:  // Input 10
      return 10;
    case 11:  // Input 11
      return 11;
    case 12:  // Input 12
      return 12;
    case 13:  // Input 13
      return 13;
    case 14:  // Input 14
      return 14;
    case 15:  // Input 15
      return 15;
    case 16:  // Input 16
      return 16;
    case 17:  // Input 17
      return 17;
    case 18:  // Input 18
      return 18;
    case 19:  // Input 19
      return 19;
    case 20:  // Input 20
      return 20;
    case 21:  // Color Bars
      return 1000;
    case 22:  // Color 1
      return 2001;
    case 23:  // Color 2
      return 2002;
    case 24:  // Media Player 1
      return 3010;
    case 25:  // Media Player 1 Key
      return 3011;
    case 26:  // Media Player 2
      return 3020;
    case 27:  // Media Player 2 Key
      return 3021;
    case 28:  // Key 1 Mask
      return 4010;
    case 29:  // Key 2 Mask
      return 4020;
    case 30:  // Key 3 Mask
      return 4030;
    case 31:  // Key 4 Mask
      return 4040;
    case 32:  // DSK 1 Mask
      return 5010;
    case 33:  // DSK 2 Mask
      return 5020;
    case 34:  // Super Source
      return 6000;
    case 35:  // Clean Feed 1
      return 7001;
    case 36:  // Clean Feed 2
      return 7002;
    case 37:  // Auxilary 1
      return 8001;
    case 38:  // Auxilary 2
      return 8002;
    case 39:  // Auxilary 3
      return 8003;
    case 40:  // Auxilary 4
      return 8004;
    case 41:  // Auxilary 5
      return 8005;
    case 42:  // Auxilary 6
      return 8006;
    case 43:  // ME 1 Prog
      return 10010;
    case 44:  // ME 1 Prev
      return 10011;
    case 45:  // ME 2 Prog
      return 10020;
    case 46:  // ME 2 Prev
      return 10021;
    default:
      return 0;
  }
}

/*
 * Translating a index to a audio source
 */
uint16_t ATEMbase::getAudioIndexSrc(uint8_t index)	{
  switch (index) {
    case 0:  // Input 1
      return 1;
    case 1:  // Input 2
      return 2;
    case 2:  // Input 3
      return 3;
    case 3:  // Input 4
      return 4;
    case 4:  // Input 5
      return 5;
    case 5:  // Input 6
      return 6;
    case 6:  // Input 7
      return 7;
    case 7:  // Input 8
      return 8;
    case 8:  // Input 9
      return 9;
    case 9:  // Input 10
      return 10;
    case 10:  // Input 11
      return 11;
    case 11:  // Input 12
      return 12;
    case 12:  // Input 13
      return 13;
    case 13:  // Input 14
      return 14;
    case 14:  // Input 15
      return 15;
    case 15:  // Input 16
      return 16;
    case 16:  // Input 17
      return 17;
    case 17:  // Input 18
      return 18;
    case 18:  // Input 19
      return 19;
    case 19:  // Input 20
      return 20;
    case 20:  // XLR
      return 1001;
    case 21:  // AES/EBU
      return 1101;
    case 22:  // RCA
      return 1201;
    case 23:  // MP1
      return 2001;
    case 24:  // MP2
      return 2002;
    default:
      return 0;
  }
}

uint8_t ATEMbase::maxAtemSeriesVideoInputs()	{
	return 47;	// For the largest ATEM switcher, this is the number of video inputs. The max "index" number from the list above
}


void ATEMbase::commandBundleStart()	{
	resetCommandBundle();
	_wipeCleanPacketBuffer();
	_cBundle = true;
}
void ATEMbase::commandBundleEnd()	{
	if (_cBundle && _returnPacketLength > 0)	{

  	  _createCommandHeader(ATEM_headerCmd_AckRequest, _returnPacketLength);
  	  _sendPacketBuffer(_returnPacketLength);
  	  _returnPacketLength = 0;
	}
	resetCommandBundle();
}
void ATEMbase::resetCommandBundle()	{
	_cBundle = false;
	_cBBO = 0;
}
