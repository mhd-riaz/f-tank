#include <Wire.h>
#include "RTClib.h"
#include <U8g2lib.h>
#include <OneWire.h>

// Constants for relay states
const bool ON = false;
const bool OFF = true;

// Appliance relay pins
const uint8_t RELAY_CO2 = 7;
const uint8_t RELAY_MOTOR = 6;
const uint8_t RELAY_HEATER = 5;
const uint8_t RELAY_LIGHT = 4;
const uint8_t BUZZER = 8;
const uint8_t TEMP_SENSOR = 10;

// RTC, Temp Sensor, and OLED initialization
RTC_DS1307 rtc;
OneWire ds(TEMP_SENSOR);
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

// Schedule structure
struct Schedule
{
    uint16_t start; // Start time in minutes
    uint16_t end;   // End time in minutes
};

// Schedules for appliances
// LIGHT - morning [10:30 - 2:30 = 4 hrs], afternoon [5:00 - 8:00 = 3hrs]         // TOTAL LIGHT  = 7 Hrs
const Schedule lightSchedule[] = {{630, 870}, {1020, 1200}};
// CO2 - morning [7:30 - 1:30 = 6 hrs], afternoon [2:30 - 8:30 = 6 hrs]         // TOTAL CO2    = 12 Hrs
const Schedule co2Schedule[] = {{450, 810}, {870, 1230}};
// HEATER - mid night [0:00 - 4:00 = 4 hrs], noon [2:31 - 4:31 = 2hr]        // TOTAL HEATER = ~6 Hrs
const Schedule heaterSchedule[] = {{0, 240}, {871, 991}};
// MOTOR - ONLY switch off from [9:30 AM - 10:30 = 1 hr] & [2:00 PM - 3:00 = 1hr] // TOTAL MOTOR  = 24 - 2 = 22 Hrs
const Schedule motorOffSchedule[] = {{570, 630}, {840, 900}};

// Appliance states
struct ApplianceState
{
    bool light = OFF;
    bool co2 = OFF;
    bool heater = OFF;
    bool motor = OFF;
} applianceState;

// Buffers for OLED display
char timestampBuffer[16];
char logBuffer[32];

void setup()
{
    setupRelays();
    buzzerBeep();
    Serial.begin(9600);
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x12_tf);
    initRtc();
}

unsigned long lastTemperatureRead = 0;
uint16_t heaterTimer = 0; // heater balance
float temp = -127.0;      // Default value indicating no valid reading

void loop()
{
    delay(1500);

    DateTime now = rtc.now();
    uint16_t nowInMinutes = getTimeInMinutes(now);

    // Non-blocking temperature read every 7*750ms seconds
    if ((millis() - lastTemperatureRead >= 5250) || (lastTemperatureRead == 0))
    {
        temp = getTemperature();
        lastTemperatureRead = millis();
    }

    // Update appliance states
    int heaterBalance = updateApplianceStates(nowInMinutes);

    // Update display buffers
    snprintf(timestampBuffer, sizeof(timestampBuffer), "%02d:%02d:%02d-%2d/%2d", now.hour(), now.minute(), now.second(), now.day(), now.month());
    if (now.second() % 10 > 5)
    {
        snprintf(logBuffer, sizeof(logBuffer), "C:%d H:%d m:%d L:%d %d", applianceState.co2, applianceState.heater, applianceState.motor, applianceState.light, nowInMinutes);
    }
    else
    {
        String tempStr = "Temperature: " + String(temp, 2) + " °C";
        snprintf(logBuffer, 20, tempStr.c_str());
        if (heaterBalance > 0)
        {
            snprintf(timestampBuffer, sizeof(timestampBuffer), "Heater ON: %d", heaterBalance);
        }
    }

    // Up OLED display
    updateDisplay();

    // Log appliance states to Serial Monitor
    logApplianceStates(now);
}

void setupRelays()
{
    uint8_t pins[] = {RELAY_LIGHT, RELAY_HEATER, RELAY_MOTOR, RELAY_CO2};
    for (uint8_t pin : pins)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, OFF);
    }
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, false);
}

void buzzerBeep()
{
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);
}

void initRtc()
{
    if (!rtc.begin())
    {
        Serial.println(F("RTC initialization failed!"));
        while (true)
            digitalWrite(BUZZER, HIGH);
    }
    if (!rtc.isrunning())
    {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        buzzerBeep();
    }
}

uint16_t getTimeInMinutes(DateTime now)
{
    return now.hour() * 60 + now.minute();
}

bool isWithinSchedule(uint16_t nowInMinutes, const Schedule *schedules, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
    {
        if (nowInMinutes >= schedules[i].start && nowInMinutes <= schedules[i].end)
        {
            return true;
        }
    }
    return false;
}

int updateApplianceStates(uint16_t nowInMinutes)
{
    applianceState.light = isWithinSchedule(nowInMinutes, lightSchedule, sizeof(lightSchedule) / sizeof(Schedule));
    applianceState.co2 = isWithinSchedule(nowInMinutes, co2Schedule, sizeof(co2Schedule) / sizeof(Schedule));
    applianceState.heater = isWithinSchedule(nowInMinutes, heaterSchedule, sizeof(heaterSchedule) / sizeof(Schedule));
    applianceState.motor = !isWithinSchedule(nowInMinutes, motorOffSchedule, sizeof(motorOffSchedule) / sizeof(Schedule));
    digitalWrite(RELAY_LIGHT, applianceState.light ? ON : OFF);
    digitalWrite(RELAY_CO2, applianceState.co2 ? ON : OFF);
    digitalWrite(RELAY_HEATER, applianceState.heater ? ON : OFF);
    digitalWrite(RELAY_MOTOR, applianceState.motor ? ON : OFF);
    return 0;
}

void logApplianceStates(DateTime now)
{
    Serial.print(now.timestamp(DateTime::TIMESTAMP_TIME));
    Serial.print(", ");
    Serial.print(applianceState.co2 ? "C:1" : "C:0");
    Serial.print(", ");
    Serial.print(applianceState.heater ? "H:1" : "H:0");
    Serial.print(", ");
    Serial.print(applianceState.motor ? "M:1" : "M:0");
    Serial.print(", ");
    Serial.println(applianceState.light ? "L:1" : "L:0");
}

float getTemperature()
{
    byte data[9], addr[8];
    if (!ds.search(addr))
    {
        ds.reset_search();
        return -127.0;
    }

    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);
    delay(750);

    ds.reset();
    ds.select(addr);
    ds.write(0xBE);
    for (int i = 0; i < 9; i++)
        data[i] = ds.read();
    ds.reset_search();

    int16_t rawTemperature = (data[1] << 8) | data[0];
    return rawTemperature & 0x8000 ? -((~rawTemperature + 1) * 0.0625) : rawTemperature * 0.0625;
}

void updateDisplay()
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.setCursor(0, 16);
    u8g2.print(timestampBuffer);

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setCursor(0, 32);
    u8g2.print(logBuffer);

    u8g2.sendBuffer();
}
