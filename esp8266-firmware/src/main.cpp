#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <aREST.h>
#define FASTLED_ESP8266_DMA
// #define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERNAL
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include "Ticker.h"
#include "crc16.h"

// Eeprom version (this needs to be the same with the EEPROM's)
enum en_eeprom_versions {
    EE_VERSION_0,
};

// Default configuration values
#define FLASH_PREAMBLE          0xBEEF
#define DEF_LISTEN_PORT         80
#define MAX_SSID_LEN            30
#define MAX_SSID_PASSWD_LEN     30
#define DEF_LED_ON_TIMEOUT_MS   5000
#define DEF_CRC_POLY            0xFFFF
#define RESET_TO_DEF_PIN        5   // D1 on nodemcu esp8266
char * def_ssid = "DTLB";
char * def_ssid_password = "RTFMYFM78";
CRGB def_led_on_color = CRGB::White;
CRGB def_led_off_color = CRGB::Black;

/* RGB WS2812B */
#define STRIPE_CHIPSET WS2812B
#define STRIPE_DATA_PIN  4     // D2 on nodemcu esp8266
#define STRIPE_COLOR_ORDER GRB
#define NUM_LEDS    100

#define BRIGHTNESS  255
#define FRAMES_PER_SECOND 1

// Create aREST instance
aREST rest = aREST();

struct tp_config {
    uint16_t    preamble;
    uint8_t     version;
    uint16_t    length;
    char        ssid[MAX_SSID_LEN];
    char        ssid_password[MAX_SSID_PASSWD_LEN];
    int         server_port;
    CRGB        led_on_color;
    CRGB        led_off_color;
    int         led_on_timeout;
    CRGB        led_ambient;
    uint8_t     enable_ambient;
    uint16_t    crc;
};
struct tp_config config;

bool save_to_eeprom = false;

// Create an instance of the server
// WiFiServer server(LISTEN_PORT);
WiFiServer * server;
CRGB leds[NUM_LEDS];        // LED colors
uint8_t leds_tmr[NUM_LEDS];  // LED timers. 0: disabled, >100s of ms

// Ticker timer
#define MAIN_TIMER_TIMEOUT_MS 100
int tmr_100ms;
Ticker main_tmr;
Ticker debug_tmr;

// Function prototypes
int restapi_led_control(String command);
int restapi_led_on_color(String command);
int restapi_led_off_color(String command);
int restapi_led_on_timeout(String command);
int restapi_wifi_ssid(String command);
int restapi_wifi_password(String command);
int restapi_led_ambient(String command);
int restapi_enable_ambient(String command);

void load_default_configuration(struct tp_config * cfg)
{
    printf("Loading defaults\n");
    cfg->preamble = FLASH_PREAMBLE;
    cfg->version = EE_VERSION_0;
    cfg->length = sizeof(struct tp_config);
    memcpy(cfg->ssid, def_ssid, strlen(def_ssid));
    cfg->ssid[strlen(def_ssid)] = 0;
    memcpy(cfg->ssid_password, def_ssid_password, strlen(def_ssid_password));
    cfg->ssid_password[strlen(def_ssid_password)] = 0;
    cfg->server_port = DEF_LISTEN_PORT;
    cfg->led_on_color = def_led_on_color;
    cfg->led_off_color = def_led_off_color;
    cfg->led_on_timeout = DEF_LED_ON_TIMEOUT_MS;
    cfg->led_ambient = CRGB::Red;
    cfg->enable_ambient = 0;
    cfg->crc = crc16((uint8_t*)cfg, sizeof(struct tp_config) - sizeof(uint16_t), DEF_CRC_POLY);
    printf("Calculated CRC16: 0x%04X\n", cfg->crc);
}

int check_eeprom_version(struct tp_config * cfg)
{
    int resp = 0;
    switch (cfg->version) {
        case EE_VERSION_0:
            // Current supported version nothing to do
            break;
        default:
            printf("Couldn't find a valid EERPOM version.\n");
            load_default_configuration(cfg);
            break;
    }
    return resp;
}

void setup() {
    // Start Serial
    Serial.begin(115200);
    delay(500);

    printf("Program started\n");

    // This is an input
    pinMode(RESET_TO_DEF_PIN, INPUT);

    // Load config from EEPROM
    EEPROM.begin(sizeof(struct tp_config));
    EEPROM.get(0, config);
    // Check for reset to defaults pin. 0=no, 1=reset EEPROM
    if (digitalRead(RESET_TO_DEF_PIN)) {
        printf("Force reset to defaults...\n");
        load_default_configuration(&config);
    }
    // Check for valid data
    else if (config.preamble != FLASH_PREAMBLE) {
        printf("Didn't found valid data...\n");
        load_default_configuration(&config);
    }
    else {
        printf("Valid EEPROM found. Checking version...\n");
        // Check correct EEPROM version
        check_eeprom_version(&config);
    }
    EEPROM.put(0, config);
    EEPROM.commit();
    EEPROM.end();

    // aRest functions to be exposed
    rest.function("led_index", restapi_led_control);
    rest.function("led_on_color", restapi_led_on_color);
    rest.function("led_off_color", restapi_led_off_color);
    rest.function("led_on_timeout", restapi_led_on_timeout);
    rest.function("led_ambient", restapi_led_ambient);
    rest.function("enable_ambient", restapi_enable_ambient);
    rest.function("wifi_ssid", restapi_wifi_ssid);
    rest.function("wifi_password", restapi_wifi_password);
    // aREST variables
    rest.variable("led_on_color", &config.led_on_color);
    rest.variable("led_off_color", &config.led_off_color);
    rest.variable("led_on_timeout", &config.led_on_timeout);
    rest.variable("led_ambient", &config.led_ambient);
    rest.variable("enable_ambient", &config.enable_ambient);
    rest.variable("wifi_ssid", &config.ssid);
    rest.variable("wifi_password",&config.ssid_password);

    // Give name & ID to the device (ID should be 6 characters long)
    rest.set_id("1");
    rest.set_name("esp8266");

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    // WiFi.setOutputPower(12.5);
    WiFi.begin(config.ssid, config.ssid_password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    // Start the server
    server = new WiFiServer(config.server_port);
    server->begin();
    Serial.println("Server started");

    // Print the IP address
    Serial.println(WiFi.localIP());

    main_tmr.attach_ms(MAIN_TIMER_TIMEOUT_MS, []() {
        tmr_100ms++;
    });

    // Setup WS2812B
    FastLED.addLeds<STRIPE_CHIPSET, STRIPE_DATA_PIN, STRIPE_COLOR_ORDER>
        (leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(BRIGHTNESS);

    for (int i=0; i<NUM_LEDS; i++) {
        leds[i] = config.led_off_color;
        leds_tmr[i] = 0;
    }
    FastLED.show();
}

uint16_t tick = 0;
void loop() {
    // Handle REST calls
    WiFiClient client = server->available();
    if (client) {
        printf("client\n");
        while(!client.available()){
            delay(5);
            // add a timeout here.
            if ((tick++) > 50000) {
                tick = 0;
                break;
            }
        }
        rest.handle(client);
    }
    if (save_to_eeprom) {
        save_to_eeprom = false;
        printf("Saving configuration\n");
        EEPROM.begin(sizeof(struct tp_config));
        EEPROM.put(0, config);
        EEPROM.commit();
        EEPROM.end();
    }
    if (tmr_100ms) {
        bool update = false;
        tmr_100ms = 0;
        for (int i=0; i<NUM_LEDS; i++) {
            // LED is enabled but timer not expired
            if (leds_tmr[i] > 1) {
                leds_tmr[i]--;
            }
            // Timer is expired
            else if (leds_tmr[i] == 1) {
                // turn off LED
                printf("Led[%d] off\n", i);
                leds[i] = config.led_off_color;
                update = true;
                leds_tmr[i] = 0;    // disable timer
            }
        }
        if (update) {
            FastLED.show();
        }
    }
}

// Custom function accessible by the API
ICACHE_RAM_ATTR int restapi_led_control(String command)
{
    // Get state from command
    int led_index = command.toInt();
    if (led_index < NUM_LEDS) {
        leds[led_index] = config.led_on_color;
        leds_tmr[led_index] = (uint8_t)(config.led_on_timeout / MAIN_TIMER_TIMEOUT_MS) + 1;
        FastLED.show();
        printf("Led[%d] on\n", led_index);
    }
    return 0;
}

ICACHE_RAM_ATTR int restapi_led_on_color(String command)
{
    config.led_on_color = command.toInt();
    printf("[aREST] setting LED on color to: %d\n", (int) config.led_on_color);
    save_to_eeprom = true;
    return 0;
}

ICACHE_RAM_ATTR int restapi_led_off_color(String command)
{
    config.led_off_color = command.toInt();
    printf("[aREST] setting LED off color to: %d\n", (int) config.led_off_color);
    save_to_eeprom = true;
    return 0;
}

ICACHE_RAM_ATTR int restapi_led_on_timeout(String command)
{
    config.led_on_timeout = command.toInt();
    printf("[aREST] setting LED on timeout to: %d\n", (int) config.led_on_timeout);
    save_to_eeprom = true;
    return 0;
}

ICACHE_RAM_ATTR int restapi_led_ambient(String command)
{
    config.led_ambient = command.toInt();
    printf("[aREST] setting LED ambient to: %d\n", (int) config.led_ambient);
    save_to_eeprom = true;

    // Update color
    if (config.enable_ambient) {
        for (int i=0; i<NUM_LEDS; i++) {
            leds[i] = config.led_ambient;;
        }
        FastLED.show();
    }
    return 0;
}

ICACHE_RAM_ATTR int restapi_enable_ambient(String command)
{
    CRGB color = CRGB::Black;

    config.enable_ambient = command.toInt();
    printf("[aREST] Enabling LED ambient: %d\n", (int) config.enable_ambient);

    if (config.enable_ambient) {
        color = config.led_ambient;
    }
    for (int i=0; i<NUM_LEDS; i++) {
        leds[i] = color;
    }
    FastLED.show();
    return 0;
}

ICACHE_RAM_ATTR int restapi_wifi_ssid(String command)
{
    printf("[aREST] setting ssid to: %s\n", command.c_str());
    strcpy(config.ssid, command.c_str());
    save_to_eeprom = true;
    return 0;
}

ICACHE_RAM_ATTR int restapi_wifi_password(String command)
{
    printf("[aREST] setting ssid password to: %s\n", command.c_str());
    strcpy(config.ssid_password, command.c_str());
    save_to_eeprom = true;
    return 0;
}