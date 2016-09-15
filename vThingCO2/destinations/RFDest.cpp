#include "interfaces\Destination.hpp"
#include "interfaces\Plugin.hpp"
#include <LinkedList.h>
#include "interfaces\Pair.h"
#include "RFDest.hpp"
#include "common.hpp"
#include "MenuHandler.hpp"
#include "plugins\PropertyList.hpp"
#include "plugins\AT_FW_Plugin.hpp"

RFDest::RFDest() {
  registerDestination(this);
}

void RFDest::setup(MenuHandler *handler) {
  handler->registerCommand(new MenuEntry(F("rf_setaddr"), CMD_BEGIN, &RFDest::cmdSetAddr, F("rf_setaddr \"CO2\" \"136\"")));
  checkIfDefaultsAreSet();
  enabled = PropertyList.readBoolProperty(F("rf.enabled"));
  if (enabled) rfBegin(port, 0, 1);
}

void RFDest::checkIfDefaultsAreSet() {
  if (!PropertyList.hasProperty(F("rf.CO2")))  PropertyList.putProperty(F("rf.CO2"),  F("136"));
  if (!PropertyList.hasProperty(F("rf.PM25"))) PropertyList.putProperty(F("rf.PM25"), F("137"));
  if (!PropertyList.hasProperty(F("rf.PM10"))) PropertyList.putProperty(F("rf.CO2"),  F("138"));
  if (!PropertyList.hasProperty(F("rf.TEMP"))) PropertyList.putProperty(F("rf.TEMP"), F("139"));
  if (!PropertyList.hasProperty(F("rf.HUM")))  PropertyList.putProperty(F("rf.HUM"),  F("140"));
  if (!PropertyList.hasProperty(F("rf.PRES"))) PropertyList.putProperty(F("rf.PRES"), F("141"));
  if (!PropertyList.hasProperty(F("rf.GEN")))  PropertyList.putProperty(F("rf.GEN"),  F("142"));
}

void RFDest::cmdSetAddr(const char *line) {
  char key[20], addr[20];
  line = extractStringFromQuotes(line, key,  sizeof(key));
  line = extractStringFromQuotes(line, addr, sizeof(addr));
  String s = F("rf.");
  s += key;
  PropertyList.putProperty(s.c_str(), addr);
  Serial << F("Set RF Address: ") << key << F("=") << addr << endl;;
}

void RFDest::process(LinkedList<Pair *> &data) {
  Serial << F("RFDest::process") << endl;
  if (!enabled) return;
  for (int i=0; i < data.size(); i++) {
    Pair *p = data.get(i);
    String s = F("rf.");
    s += p->key;
    String saddr = PropertyList.readProperty(s);
    if (saddr.length() == 0) saddr = PropertyList.readProperty(F("rf.GEN"));
    int addr = atoi(saddr.c_str());
    if (addr < 0) continue;
    long value = atof(p->value.c_str()) * 100;
    if (DEBUG) Serial.printf(String(F("RF X10 Meter: addr=%d, value=%d\n")).c_str(), addr, value);
    RFXmeter(addr, 0, value);
  }
}

void RFDest::rfBegin(uint8_t tx_pin, uint8_t led_pin, uint8_t rf_repeats) {
    pinMode(_tx_pin, OUTPUT);
   	if (_led_pin > 0) pinMode(_led_pin, OUTPUT);
		_tx_pin = tx_pin;
		_led_pin = led_pin;
		_rf_repeats = rf_repeats;
}

void RFDest::RFXmeter(uint8_t rfxm_address, uint8_t rfxm_packet_type, long rfxm_value){
	uint8_t x10buff[5]; // Set message buffer
	x10buff[0] = rfxm_address;
	x10buff[1] = (~x10buff[0] & 0xF0) + (x10buff[0] & 0xF); // Calculate byte1 (byte 1 complement upper nibble of byte0)
	if (rfxm_value > 0xFFFFFF) rfxm_value = 0; 	// We only have 3 byte for data. Is overflowed set to 0
	// Packet type goed into MSB nibble of byte 5. Max 15 (B1111) allowed
	// Use switch case to filter invalid data types
	switch(rfxm_packet_type) {
		case 0x00: //Normal. Put counter values in byte 4,2 and 3
			x10buff[4] = (uint8_t) ((rfxm_value >> 16) & 0xff);
			x10buff[2] = (uint8_t) ((rfxm_value >> 8) & 0xff);
			x10buff[3] = (uint8_t) (rfxm_value & 0xff);
			break;
		case 0x01: // New interval time set. Byte 2 should be filled with interval
			switch(rfxm_value) {
				case 0x01: break;	// 30sec
				case 0x02: break;	// 01min
				case 0x04: break;	// 06min (RFXpower = 05min)
				case 0x08: break;	// 12min (RFXpower = 10min)
				case 0x10: break; 	// 15min
				case 0x20: break; 	// 30min
				case 0x40: break; 	// 45min
				case 0x80: break; 	// 60min
				default:
				rfxm_value = 0x01; // Set to 30 sec if no valid option is found
			}
			x10buff[2] = rfxm_value;
			break;
		case 0x02: // calibrate value in <counter value> in µsec.
			x10buff[4] = (uint8_t) ((rfxm_value >> 16) & 0xff);
			x10buff[2] = (uint8_t) ((rfxm_value >> 8) & 0xff);
			x10buff[3] = (uint8_t) (rfxm_value & 0xff);
			break;
		case 0x03: break;// new address set
		case 0x04: break; // counter value reset to zero
		case 0x0B: // counter value set
			x10buff[4] = (uint8_t) ((rfxm_value >> 16) & 0xff);
			x10buff[2] = (uint8_t) ((rfxm_value >> 8) & 0xff);
			x10buff[3] = (uint8_t) (rfxm_value & 0xff);
			break;
		case 0x0C: break; // set interval mode within 5 seconds
		case 0x0D: break; // calibration mode within 5 seconds
		case 0x0E: break; // set address mode within 5 seconds
		case 0x0F: // identification packet (byte 2 = address, byte 3 = interval)
			switch(rfxm_value) {
				case 0x01: break;	//30sec
				case 0x02: break;	//01min
				case 0x04: break;	//06min (RFXpower = 05min)
				case 0x08: break;	//12min (RFXpower = 10min)
				case 0x10: break; // 15min1
				case 0x20: break; // 30min
				case 0x40: break; // 45min
				case 0x80: break; // 60min
				default:
				rfxm_value = 0x01; // Set to 30 sec if no valid option is found
			}
			x10buff[2] = rfxm_address;
			x10buff[3] = rfxm_value;
			break;
		default: //Unknown packet type. Set packet type to zero and set counter to rfxm_value
			rfxm_packet_type = 0;
			x10buff[4] = (uint8_t) ((rfxm_value >> 16) & 0xff);
			x10buff[2] = (uint8_t) ((rfxm_value >> 8) & 0xff);
			x10buff[3] = (uint8_t) (rfxm_value & 0xff);
	}
	x10buff[5] = (rfxm_packet_type << 4); 		// Packet type goes into byte 5's upper nibble.
	// Calculate parity which
	uint8_t parity = ~(((x10buff[0] & 0XF0) >> 4) + (x10buff[0] & 0XF) + ((x10buff[1] & 0XF0) >> 4) + (x10buff[1] & 0XF) + ((x10buff[2] & 0XF0) >> 4) + (x10buff[2] & 0XF) + ((x10buff[3] & 0XF0) >> 4) + (x10buff[3] & 0XF) + ((x10buff[4] & 0XF0) >> 4) + (x10buff[4] & 0XF) + ((x10buff[5] & 0XF0) >> 4));
	x10buff[5] = (x10buff[5] & 0xf0) + (parity & 0XF);
	SendCommand(x10buff, sizeof(x10buff)); // Send byte to be broadcasted
}

void RFDest::SendCommand(uint8_t *data, uint8_t size){
	if (_led_pin > 0) digitalWrite(_led_pin, HIGH);
	for (int i = 0; i < _rf_repeats; i++){
		SEND_HIGH();delayMicroseconds(X10_RF_SB_LONG);
		SEND_LOW();delayMicroseconds(X10_RF_SB_SHORT);
		for(int i=0; i <= size; i++) {
			SendX10RfByte(data[i]);
		}
	SendX10RfBit(1);
	delayMicroseconds(X10_RF_GAP);
	}
	if (_led_pin > 0) digitalWrite(_led_pin, LOW);
}

void RFDest::SendX10RfByte(uint8_t data){
	int i = 0;
	//Serial.println("\n");
	for (int i=7; i >= 0 ; i--){ // send bits from byte
		SendX10RfBit((bitRead(data,i)==1));
		//Serial.print(bitRead(data,i));
	}
}

void RFDest::SendX10RfBit(unsigned int databit){
    SEND_HIGH();delayMicroseconds(X10_RF_BIT_SHORT);
    SEND_LOW();delayMicroseconds(X10_RF_BIT_SHORT);
    if (databit) delayMicroseconds(X10_RF_BIT_LONG);
}

void RFDest::SEND_HIGH() {
	digitalWrite(_tx_pin, HIGH);
}

void RFDest::SEND_LOW(){
	digitalWrite(_tx_pin, LOW);
}