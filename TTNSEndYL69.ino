#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// 1. SENSOR CONFIGURATION (YL-69)

#define SOIL_PIN 34  


const int AIR_VALUE = 4095;
const int WATER_VALUE = 1500;


#define OLED_SDA 4    
#define OLED_SCL 15   
#define OLED_RST 16   
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RST);


// 2. LORA KEYS (FILL THESE IN!)

static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// 2. DEVEUI - FORMAT: LSB 
static const u1_t PROGMEM DEVEUI[8] = { 0x77, 0x44, 0x07, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 }; 
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// 3. APPKEY - FORMAT: MSB 
static const u1_t PROGMEM APPKEY[16] = { 0x1D, 0xF4, 0x49, 0x3D, 0x1E, 0x85, 0x91, 0xBD, 0xF6, 0x99, 0xBF, 0xE9, 0x3C, 0xDE, 0x8F, 0x6C }; 
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16);}

// -----------------------------------------------------------------------------
// 4. PIN MAPPING (TTGO V1)
// -----------------------------------------------------------------------------
const lmic_pinmap lmic_pins = {
    .nss = 18,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 14,
    .dio = {26, 33, 32} 
};

static osjob_t sendjob;
const unsigned TX_INTERVAL = 60; 

void updateDisplay(String line1, String line2) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(F("Soil Monitor"));
    display.println("-------------");
    display.println(line1);
    display.println(line2);
    display.display();
}

void do_send(osjob_t* j){
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        
        int rawValue = analogRead(SOIL_PIN);
        
        
        int percent = map(rawValue, AIR_VALUE, WATER_VALUE, 0, 100);
        
        
        if(percent < 0) percent = 0;
        if(percent > 100) percent = 100;

        
        String rawStr = "Raw: " + String(rawValue);
        String perStr = "Hum: " + String(percent) + "%";
        
        Serial.print(rawStr); Serial.print(" | "); Serial.println(perStr);
        updateDisplay(rawStr, perStr);

        
        uint8_t payload[2];
        payload[0] = highByte(percent);
        payload[1] = lowByte(percent);

        
        LMIC_setTxData2(1, payload, sizeof(payload), 0);
        Serial.println(F("Packet queued"));
    }
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            updateDisplay("Network:", "Joining...");
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            updateDisplay("Network:", "Joined!");
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            updateDisplay("Join", "Failed");
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE"));
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void setup() {
    Serial.begin(115200);
    
    
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(20);
    digitalWrite(OLED_RST, HIGH);

    
    SPI.begin(5, 19, 27, 18); 

    
    Wire.begin(OLED_SDA, OLED_SCL); 

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) { 
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.println(F("Init OK..."));
    display.display();

    
    os_init();
    LMIC_reset();
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}