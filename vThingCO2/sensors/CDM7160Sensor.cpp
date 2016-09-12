#include <Wire.h>
#include "sensors\CDM7160Sensor.hpp"
#include <LinkedList.h>
#include "interfaces\Pair.h"
#include "common.hpp"
#include  <brzo_i2c.h>
#include "utils\RTCMemStore.hpp"
CDM7160Sensor::CDM7160Sensor() {
  registerSensor(this);
}

void CDM7160Sensor::setup(MenuHandler *handler) {
  handler->registerCommand(new MenuEntry(F("cdmloop"), CMD_BEGIN, &CDM7160Sensor::onCmdLoop, F("cdmtest b - b for DEBUG - test CDM7160 sensor")));
  handler->registerCommand(new MenuEntry(F("cdmtest"), CMD_BEGIN, &CDM7160Sensor::onCmdTest, F("cdmtest b - b for DEBUG - test CDM7160 sensor")));
  handler->registerCommand(new MenuEntry(F("cdmreg"), CMD_BEGIN, &CDM7160Sensor::onChangeReg, F("cdmreg \"regid\" \"value\"")));
  if (checkI2CDevice(CDM_ADDR_WRITE)) {
    Serial << "Found" << endl;
    configureSensor();
    hasSensor = true;
  }
}

void CDM7160Sensor::getData(LinkedList<Pair *> *data) {
  if (!hasSensor) return;

  //if (!checkI2CDevice(CDM_ADDR_WRITE)) return;
  int ppm = readCO2AutoRecover();
  if (ppm > 0) {
    rtcMemStore.addAverageValue(ppm);
    data->add(new Pair("CO2", String(rtcMemStore.getAverage())));
  }
}

int CDM7160Sensor::readCO2AutoRecover() {
  int ppm=0;
  for (int i=0; i < 2; i++) {
    int a = readCO2Raw();
    Serial <<"Raw CO2: " << a << endl;
    if (a > 0) return a;
    else if (a == -1) { //maybe some temprary error, retry once more before reseting bus
      delay(2000);
    } else if (a == -2) { // cannot recover that fast, e.g. averaging not completed
      return -2;
    }
  }
  menuHandler.scheduleCommand("scani2c");
  return -1;
}

void CDM7160Sensor::configureSensor() {
  if (Wire.status() != I2C_OK) {
    Serial << F("I2C Bus status: ") << Wire.status() << endl;
    return;
  }
  uint8_t ctl = readI2CByte(CDM_CTL_REG);
  uint8_t avg = readI2CByte(CDM_AVG_REG);
  if ((ctl & CDM_FMODE) > 0)  writeByte(CDM_CTL_REG, 0x02);
  if (avg != CDM_AVG_DEFAULT) writeByte(CDM_AVG_REG, CDM_AVG_DEFAULT);
  if ((ctl & CDM_FMODE) > 0 || avg != CDM_AVG_DEFAULT) {
    Serial << F("Sensor Configuration updated") << endl;
  }
}

void CDM7160Sensor::onCmdLoop(const char *ignore) {
  Serial << "Avg count: " << readI2CByte(7) << endl;
  int last = 0;
  for (int i=0; i < 50; i++) {
    int co2 = readCO2Raw();
    Serial.printf("%2d: %d (%d)\n", i, co2, co2-last);
    last = co2;
    delay(3000);
    //menuHandler.processUserInput();
    menuHandler.loop();
    //Serial << i << "\t: " << readCO2Raw(true) << endl;
  }
}
void CDM7160Sensor::onCmdTest(const char *ignore) {
  uint8_t rst = readI2CByte(0);
  Serial << "rst is: " << _HEX(rst) << endl;
  // if (rst == (uint8_t)0xFF) {
  //   Serial << " FAILED TO COMMUNICATE WITH SENSOR. Restarting I2C" << endl;
  //   menuHandler.scheduleCommand("scani2c");
  //Serial << " Wire.status = " << Wire.status() <<endl;
  Serial << "Register 0x00 : " << _BIN(readI2CByte(0)) << endl;
  delay(100);
  //Serial << " Wire.status = " << Wire.status() <<endl;
  Serial << "Register 0x01 : " << _BIN(readI2CByte(1)) << endl;
  delay(100);
  //Serial << " Wire.status = " << Wire.status() <<endl;
  Serial << "Register 0x02 : " << _BIN(readI2CByte(2)) << endl;
  delay(100);
  //Serial << " Wire.status = " << Wire.status() <<endl;
  Serial << "Register 0x03 : " << _BIN(readI2CByte(3)) << endl;
  delay(100);
  //Serial << " Wire.status = " << Wire.status() <<endl;
  Serial << "Register 0x04 : " << _BIN(readI2CByte(4)) << endl;
  delay(100);
  //Serial << " Wire.status = " << Wire.status() <<endl;
  Serial << "Register 0x07 : " << _BIN(readI2CByte(7)) << endl;
  delay(100);
  //Serial << " Wire.status = " << Wire.status() <<endl;
  Serial << F("Read CO2: ")  <<  readCO2AutoRecover() << endl;;
}

void CDM7160Sensor::onChangeReg(const char *line) {
  char creg[10], cval[10];
  line = extractStringFromQuotes(line, creg);
  line = extractStringFromQuotes(line, cval);
  uint8_t reg = atoi(creg);
  uint8_t val = atoi(cval);
  uint8_t curVal =  readI2CByte(reg);
  Serial << "Before : " << _HEX(curVal) << " : " << _BIN(curVal) << endl;
  writeByte(reg, val);
  curVal =  readI2CByte(reg);
  Serial << "After : " << _HEX(curVal) << " : " << _BIN(curVal) << endl;

  onCmdTest("");
}

int CDM7160Sensor::readCO2Raw() {
  uint8_t buf[5];
  if (Wire.status() != I2C_OK) {
    if (DEBUG) Serial << F("I2C Bus status: ") << Wire.status() << endl;
    return -1;
  }
  for (int i=0; i < 3; i++) {
    if (!readI2CBytes(0, buf, sizeof(buf))) {
      if (DEBUG) Serial << F("Could not connect to Sensor") << endl;
      return -1;
    }
    if (buf[0] == (uint8_t) 0xFF) {
      if (DEBUG) Serial << F("I2C Bus failed") << endl;
      return -1;
    }
    if ((buf[2] & 0x80) > 0) {
      if (DEBUG) Serial << F("Sensor busy") << endl;
      delay(300);
      continue;
    }
    int ppm = (int)buf[4]*0xFF + buf[3];

    if ((buf[2] & 0x10) == 0) {
      if (DEBUG) Serial << F("Averaging not yet completed: ") << ppm  << endl;
      //delay(500);
      //continue;
      return -2;
    } else {
      return ppm;
    }
    //int ppm = (int)buf[4]*0xFF + buf[3];

  }
  return -2;
}

uint8_t CDM7160Sensor::readI2CByte(int reg) {
  uint8_t buf[1] = {0xFF};
  readI2CBytes(reg, buf, 1);
  return buf[0];
}

bool CDM7160Sensor::readI2CBytes(int start, uint8_t *buf, int len) {
  //return readI2CBytesBrzo(start, buf, len);
  for (int tr = 0; tr < 6; tr ++) {
    Wire.beginTransmission(CDM_ADDR_WRITE);
    Wire.write(start);
    int et = Wire.endTransmission(false);
    delay(10);
    if (et !=0) {
      Serial << et;
      delay(100);
      continue;
    }
    if (Wire.requestFrom(CDM_ADDR_WRITE, (size_t)len, (bool)false) < len) {
      Serial << "x";
      continue;
    }
    while (len -- > 0)   *(buf++) = Wire.read();
    return true;
  }
  return false;
}

bool CDM7160Sensor::readI2CBytesBrzo(int start, uint8_t *buf, int len) {
  for (int tr = 0; tr < 6; tr ++) {
    brzo_i2c_start_transaction(CDM_ADDR_WRITE, 400);
    byte xx[1];
    xx[0] = start;
    brzo_i2c_write(xx, 1, true);
    brzo_i2c_read(buf, len, false);
    int et = brzo_i2c_end_transaction();
    Serial << "brzo end: " << et <<endl;
    return true;
  }
  return false;
}

void CDM7160Sensor::writeByte(uint8_t reg, uint8_t value) {
  Serial << "WriteByte :reg: "<< reg << ", " << _BIN(value) <<endl;
  byte addr = 0x69;
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  delay(100);
  int r = Wire.endTransmission(true);
  delay(100);
  Serial << "End trans : " << r << endl;
}
