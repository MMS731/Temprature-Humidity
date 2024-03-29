/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses ABP (Activation-by-personalisation), where a DevAddr and
 * Session keys are preconfigured (unlike OTAA, where a DevEUI and
 * application key is configured, while the DevAddr and session keys are
 * assigned/generated in the over-the-air-activation procedure).
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!
 *
 * To use this sketch, first register your application and device with
 * the things network, to set or generate a DevAddr, NwkSKey and
 * AppSKey. Each device should have their own unique values for these
 * fields.
 *
 * Do not forget to define the radio type correctly in config.h.
 *
 *******************************************************************************/

//This code includes keys for all five SHT45 sensor devices 

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <GyverBME280.h>
//bool next = false;
int batv = 0, bat_status = 0;
GyverBME280 bme;


// LoRaWAN NwkSKey, network session key
// This should be in big-endian (aka msb).
static const PROGMEM u1_t NWKSKEY[16] = { 0x05, 0x88, 0x28, 0x5B, 0xF7, 0xD9, 0x14, 0x3A, 0x55, 0xA7, 0xE9, 0x66, 0xFD, 0x2F, 0x25, 0x14  };

// LoRaWAN AppSKey, application session key
// This should also be in big-endian (aka msb).
static const u1_t PROGMEM APPSKEY[16] = { 0x8F, 0x88, 0x1A, 0x73, 0xA9, 0xB6, 0xF2, 0x3E, 0x69, 0xAC, 0xD1, 0x05, 0xA9, 0x51, 0x1C, 0xAD};

// LoRaWAN end-device address (DevAddr)
// See http://thethingsnetwork.org/wiki/AddressSpace
// The library converts the address to network byte order as needed, so this should be in big-endian (aka msb) too.
static const u4_t DEVADDR = 0Xfc0096a7; // <-- Change this address for every node!
// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in arduino-lmic/project_config/lmic_project_config.h,

// Testing code sensor-01 key ends


// show debug statements; comment next line to disable debug statements
//#define DEBUG
// use low power sleep; comment next line to not use low power sleep
#define SLEEP



#ifdef COMPILE_REGRESSION_TEST
# define CFG_in866 1
#else
# warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
# define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
#endif


// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t mydata[10];
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 900;

#ifdef SLEEP
#include "LowPower.h" //https://github.com/rocketscream/Low-Power
bool next = false;
#endif

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 6,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 5,
    .dio = {2, 3, 4},
};

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
      break;
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
     // Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
    //  Serial.println(F("EV_JOINED"));
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      //Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
     // Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
     // Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.println(F("Received "));
        Serial.println(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      // Schedule next transmission
      #ifndef SLEEP
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      #else
      next = true;
      #endif
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      //Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
     // Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
     // Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
    //  Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
     // Serial.println(F("Unknown event"));
      break;
  }
}


void measure()
{
 
 int Temperature= int(100 * (bme.readTemperature()));
  int Humidity= int((100 * (bme.readHumidity())));
  int Pressure= pressureToMmHg(bme.readPressure());
  mydata[0] = highByte(Temperature);
  mydata[1] = lowByte(Temperature);
  mydata[2] = highByte(Humidity);
  mydata[3] = lowByte(Humidity);
  mydata[4] = highByte(Pressure);
  mydata[5] = lowByte(Pressure);
  Serial.println(F("Temp"));
  Serial.println(Temperature);
  Serial.println(F("Hum"));
  Serial.println(Humidity);
  Serial.println(F("Press"));
  Serial.println(Pressure);
}


/*int batteryv()
{

 float analogvalue = 0, battVolt = 0;
 
  for (byte  i = 0; i < 10; i++) {
    analogvalue += analogRead(A3);
    delay(5);
  }
  analogvalue = analogvalue / 10;
#ifdef DEBUG
  Serial.print("analogvalue= ");
  Serial.println(analogRead(A3));
#endif
  battVolt = ((analogvalue * 3.3) / 1024) * 2; //ADC voltage*Ref. Voltage/1024

  int batt_millivolt = battVolt * 100;

#ifdef DEBUG
  Serial.print("Voltage= ");
  Serial.print(battVolt);
  Serial.println("V");
#endif

   batv = batt_millivolt;
   mydata[6] = highByte(batv);
   mydata[7] = lowByte(batv);
     Serial.println(F("Battery voltage"));
  Serial.println(batv);
 
}*/

void voltage()                         
{
 
  double analogvalue = analogRead(A3);
  double bat_temp = ((analogvalue * 3.3) / 1024); //ADC voltage*Ref. Voltage/1024
  double sum = 0;
  double avg = 0;
  short int volt = 0;

  for (byte  i = 0; i < 4; i++)
  {
    sum += bat_temp * 2;
  }
  avg = (sum / 4);
  volt = avg*1000;
 
  //Serial.print(F(" Battery Voltage average: "));
  //Serial.print(avg);
  //Serial.println(F(" V avg"));
  //Serial.println(F("-------------"));


  //Serial.print(F(" Battery Voltage: "));
  //Serial.print(volt);
  //Serial.println(F(" V"));
  //Serial.println(F("-------------"));

  mydata[6]=highByte(volt);
  mydata[7]=lowByte(volt);
  Serial.println(F("Battery voltage"));
  Serial.println(batv);
  delay(1000);
  
}


void do_send(osjob_t* j){
  measure();
  voltage();
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("Starting"));

    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

  bme.setFilter(FILTER_COEF_8);
  bme.setTempOversampling(OVERSAMPLING_8);
  bme.setPressOversampling(OVERSAMPLING_16);
  bme.setStandbyTime(STANDBY_500MS);
  bme.begin();
/*  Serial.println("Adafruit SHT4x test");
  if (! sht4.begin()) {
    Serial.println("Couldn't find SHT4x");
    while (1) delay(1); 
  }*/
    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    #if defined(CFG_in866)
    // Set up the channels used by the Things Network, which corresponds
    // to the defaults of most gateways. Without this, only three base
    // channels from the LoRaWAN specification are used, which certainly
    // works, so it is good for debugging, but can overload those
    // frequencies, so be sure to configure the full frequency range of
    // your network here (unless your network autoconfigures them).
    // Setting up channels should happen after LMIC_setSession, as that
    // configures the minimal channel set.
    // NA-US channels 0-71 are configured automatically
//    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
//    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
//    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
//    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
//    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
//    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
//    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
//    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
//    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_CENTI);      // g2-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.


//SHT45 code begins
//
//    LMIC_setupChannel(0, 865062500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(1, 865232500, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_MILLI);      // g-band
//    LMIC_setupChannel(2, 865402500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(3, 865985000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(4, 866185000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(5, 866385000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(6, 866585000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(7, 866785000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band


    //#if defined(CFG_in866)
//     // Set up the channels used in your country. Three are defined by default,
//     // and they cannot be changed. Duty cycle doesn't matter, but is conventionally
//     // BAND_MILLI.
//     LMIC_setupChannel(0, 865062500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
//     LMIC_setupChannel(1, 865232500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
//     LMIC_setupChannel(2, 865402500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
//     LMIC_setupChannel(3, 865985000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
//     LMIC_setupChannel(4, 866185000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
//     LMIC_setupChannel(5, 866385000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
//     LMIC_setupChannel(6, 866585000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
//     LMIC_setupChannel(7, 866785000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
//    // ... extra definitions for channels 3..n here.
//
    //#endif
     LMIC_setupChannel(0, 865062500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
     LMIC_setupChannel(1, 865232500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
     LMIC_setupChannel(2, 865402500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
     LMIC_setupChannel(3, 865985000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
     LMIC_setupChannel(4, 866185000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
     LMIC_setupChannel(5, 866385000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
     LMIC_setupChannel(6, 866585000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
     LMIC_setupChannel(7, 866785000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
     
    #elif defined(CFG_us915)
    // NA-US channels 0-71 are configured automatically
    // but only one group of 8 should (a subband) should be active
    // TTN recommends the second sub band, 1 in a zero based count.
    // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
    LMIC_selectSubBand(1);
    #endif

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF12,14);

    // Start job
    do_send(&sendjob);
}

void loop() {
  #ifndef SLEEP

  os_runloop_once();

#else
  extern volatile unsigned long timer0_overflow_count;

  if (next == false) {

    os_runloop_once();

  } else {

    int sleepcycles = TX_INTERVAL / 8;  // calculate the number of sleepcycles (8s) given the TX_INTERVAL
#ifdef DEBUG
   // Serial.print(F("Enter sleeping for "));
   // Serial.print(sleepcycles);
    Serial.println(F(" cycles of 8 seconds"));
#endif
    Serial.flush(); // give the serial print chance to complete
    for (int i = 0; i < sleepcycles; i++) {
      // Enter power down state for 8 s with ADC and BOD module disabled
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);

      // LMIC uses micros() to keep track of the duty cycle, so
      // hack timer0_overflow for a rude adjustment:
      cli();
      timer0_overflow_count += 8 * 64 * clockCyclesPerMicrosecond();
      sei();
    }

    
#ifdef DEBUG
   // Serial.println(F("Sleep complete"));
#endif
    next = false;
    // Start job
    do_send(&sendjob);
  }

#endif
}
