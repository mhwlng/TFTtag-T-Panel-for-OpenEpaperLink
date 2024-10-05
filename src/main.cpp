#include <FS.h>

#include <Wire.h>
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include <SPI.h>
#include <WiFi.h>

#include "commstructs.h"
#include "main.h"
#include "udp.h"

#define LV_ATTRIBUTE_TICK_INC IRAM_ATTR
#define TOUCH_MODULES_CST_MUTUAL

// MicroSD
#define SD_CS 34
#define SD_SCLK 36
#define SD_MOSI 35
#define SD_MISO 37

// SPI
#define SCLK 36
#define MOSI 35

// IIC
#define IIC_SDA 17
#define IIC_SCL 18

// YDP395BT001-V2
#define LCD_WIDTH 480
#define LCD_HEIGHT 480
#define LCD_CS 14
#define LCD_DE 38
#define LCD_VSYNC 40
#define LCD_HSYNC 39
#define LCD_PCLK 41
#define LCD_B0 1
#define LCD_B1 2
#define LCD_B2 3
#define LCD_B3 4
#define LCD_B4 5
#define LCD_G0 6
#define LCD_G1 7
#define LCD_G2 8
#define LCD_G3 9
#define LCD_G4 10
#define LCD_G5 11
#define LCD_R0 12
#define LCD_R1 13
#define LCD_R2 42
#define LCD_R3 46
#define LCD_R4 45
#define LCD_BL 33


// CST3240
#define CST3240_Address 0x5A
#define TOUCH_RST 4
#define TOUCH_INT 21

// KEY
#define KEY1 48
#define KEY2 47
#define BOOT 0

const char *ssid = "xxx";
const char *password = "yyy";


static const uint8_t st7701_type9_init_operations[] = {

    BEGIN_WRITE,
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x13,

    WRITE_C8_D8, 0xEF, 0x08,

    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x10,

    WRITE_C8_D16, 0xC0, 0x3B, 0x00,
    WRITE_C8_D16, 0xC1, 0x0B, 0x02,

    WRITE_COMMAND_8, 0xC2,
    WRITE_BYTES, 3, 0x30, 0x02, 0x37,

    WRITE_C8_D8, 0xCC, 0x10,

    WRITE_COMMAND_8, 0xB0, // Positive Voltage Gamma Control
    WRITE_BYTES, 16,
    0x00, 0x0F, 0x16, 0x0E,
    0x11, 0x07, 0x09, 0x09,
    0x08, 0x23, 0x05, 0x11,
    0x0F, 0x28, 0x2D, 0x18,

    WRITE_COMMAND_8, 0xB1, // Negative Voltage Gamma Control
    WRITE_BYTES, 16,
    0x00, 0x0F, 0x16, 0x0E,
    0x11, 0x07, 0x09, 0x08,
    0x09, 0x23, 0x05, 0x11,
    0x0F, 0x28, 0x2D, 0x18,

    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x11,

    WRITE_C8_D8, 0xB0, 0x4D, 
    WRITE_C8_D8, 0xB1, 0x33, 
    WRITE_C8_D8, 0xB2, 0x87, 
    WRITE_C8_D8, 0xB5, 0x4B, 
    WRITE_C8_D8, 0xB7, 0x8C,
    WRITE_C8_D8, 0xB8, 0x20, 
    WRITE_C8_D8, 0xC1, 0x78,
    WRITE_C8_D8, 0xC2, 0x78,
    WRITE_C8_D8, 0xD0, 0x88,

    WRITE_COMMAND_8, 0xE0,
    WRITE_BYTES, 3, 0x00, 0x00, 0x02,

    WRITE_COMMAND_8, 0xE1,
    WRITE_BYTES, 11,
    0x02, 0xF0, 0x00, 0x00,
    0x03, 0xF0, 0x00, 0x00,
    0x00, 0x44, 0x44,

    WRITE_COMMAND_8, 0xE2,
    WRITE_BYTES, 12,
    0x10, 0x10, 0x40, 0x40,
    0xF2, 0xF0, 0x00, 0x00,
    0xF2, 0xF0, 0x00, 0x00,

    WRITE_COMMAND_8, 0xE3,
    WRITE_BYTES, 4, 0x00, 0x00, 0x11, 0x11,

    WRITE_C8_D16, 0xE4, 0x44, 0x44,

    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 16,
    0x07, 0xEF, 0xF0, 0xF0,
    0x09, 0xF1, 0xF0, 0xF0,
    0x03, 0xF3, 0xF0, 0xF0,
    0x05, 0xED, 0xF0, 0xF0,

    WRITE_COMMAND_8, 0xE6,
    WRITE_BYTES, 4, 0x00, 0x00, 0x11, 0x11,

    WRITE_C8_D16, 0xE7, 0x44, 0x44,

    WRITE_COMMAND_8, 0xE8,
    WRITE_BYTES, 16,
    0x08, 0xF0, 0xF0, 0xF0,
    0x0A, 0xF2, 0xF0, 0xF0,
    0x04, 0xF4, 0xF0, 0xF0,
    0x06, 0xEE, 0xF0, 0xF0,

    WRITE_COMMAND_8, 0xEB,
    WRITE_BYTES, 7,
    0x00, 0x00, 0xE4, 0xE4,
    0x44, 0x88, 0x40,

    WRITE_C8_D16, 0xEC, 0x78, 0x00,

    WRITE_COMMAND_8, 0xED,
    WRITE_BYTES, 16,
    0x20, 0xF9, 0x87, 0x76,
    0x65, 0x54, 0x4F, 0xFF,
    0xFF, 0xF4, 0x45, 0x56,
    0x67, 0x78, 0x9F, 0x02,

    WRITE_COMMAND_8, 0xEF,
    WRITE_BYTES, 6,
    0x10, 0x0D, 0x04, 0x08,
    0x3F, 0x1F,

    // WRITE_C8_D8, 0xCD, 0x05,//Test

    WRITE_C8_D8, 0x3A, 0x55,

    WRITE_C8_D8, 0x36, 0x08,

    WRITE_COMMAND_8, 0x11,

    // WRITE_COMMAND_8, 0xFF,//Test
    // WRITE_BYTES, 5,
    // 0x77, 0x01, 0x00, 0x00,
    // 0x12,

    // WRITE_C8_D8, 0xD1, 0x81,//Test
    // WRITE_C8_D8, 0xD2, 0x08,//Test

    WRITE_COMMAND_8, 0x29, // Display On

    // WRITE_C8_D8, 0x35, 0x00,//Test
    // WRITE_C8_D8, 0xCE, 0x04,//Test

    // WRITE_COMMAND_8, 0xF2,//Test
    // WRITE_BYTES, 4,
    // 0xF0, 0xA3, 0xA3, 0x71,

    END_WRITE};

Arduino_DataBus *bus = new Arduino_SWSPI(-1 /* DC */, LCD_CS /* CS */,
                                         SCLK /* SCK */, MOSI /* MOSI */, -1 /* MISO */);
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    LCD_DE /* DE */, LCD_VSYNC /* VSYNC */, LCD_HSYNC /* HSYNC */, LCD_PCLK /* PCLK */,
    LCD_B0 /* B0 */, LCD_B1 /* B1 */, LCD_B2 /* B2 */, LCD_B3 /* B3 */, LCD_B4 /* B4 */,
    LCD_G0 /* G0 */, LCD_G1 /* G1 */, LCD_G2 /* G2 */, LCD_G3 /* G3 */, LCD_G4 /* G4 */, LCD_G5 /* G5 */,
    LCD_R0 /* R0 */, LCD_R1 /* R1 */, LCD_R2 /* R2 */, LCD_R3 /* R3 */, LCD_R4 /* R4 */,
    1 /* hsync_polarity */, 20 /* hsync_front_porch */, 1 /* hsync_pulse_width */, 1 /* hsync_back_porch */,
    1 /* vsync_polarity */, 30 /* vsync_front_porch */, 1 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
    10 /* pclk_active_neg */, 6000000L /* prefer_speed */, false /* useBigEndian */,
    0 /* de_idle_high*/, 0 /* pclk_idle_high */);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    LCD_WIDTH /* width */, LCD_HEIGHT /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    bus, -1 /* RST */, st7701_type9_init_operations, sizeof(st7701_type9_init_operations));


void advertiseTagTask(void *parameter) {
	sendAvail(0xFC);
	while (true) {
		vTaskDelay(60000 / portTICK_PERIOD_MS);
		sendAvail(0);
	}
}

void drawImage(uint8_t *buffer, uint8_t dataType) {

    uint16_t* data = (uint16_t*)(buffer);
    
    for (int16_t j = 0; j < 480; j++)
    {
        for (int16_t i = 0; i < 480; i++)
        {
            uint16_t color = *data;
            color = color<<8 | color>>8;
            *data = color;
            data++;
        }
    }

    gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)buffer, 480, 480);

}

void setup() {
	Serial.begin(115200);


    pinMode(KEY1, INPUT_PULLUP);
    pinMode(KEY2, INPUT_PULLUP);

    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW);

    pinMode(TOUCH_INT, INPUT_PULLUP);
    digitalWrite(TOUCH_INT, HIGH);
   
    ledcAttachPin(LCD_BL, 1);
    ledcSetup(1, 1000, 8);
    ledcWrite(1, 255); // brightness

    Wire.begin(IIC_SDA, IIC_SCL);

    gfx->begin();

    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);

    gfx->setCursor(0, 0);
    gfx->setTextSize(2);
  
	WiFi.begin(ssid, password);

	gfx->println("");
    gfx->println("");
	gfx->println("   Connecting to WiFi");

    gfx->print("   ");
	while (WiFi.status() != WL_CONNECTED) {
		gfx->print(".");
		delay(500);
	}
	gfx->println("connected!");

    IPAddress IP = WiFi.softAPIP();
    gfx->println("   " + String(IP.toString().c_str()));

	vTaskDelay(1000 / portTICK_PERIOD_MS);

	init_udp();

	xTaskCreate(advertiseTagTask, "Tag alive", 6000, NULL, 2, NULL);
}

void loop() {
	vTaskDelay(10000 / portTICK_PERIOD_MS);
}
