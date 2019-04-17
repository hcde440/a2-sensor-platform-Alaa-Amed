// *********************************************
// ALAA'S ADAFRUIT DASHBOARD 
// https://io.adafruit.com/alaa_amed/dashboards

// *********************************************

// | Pot Names |
// Tool for generating random _names_, _surenames_, and _regions_ by turning a potentiometer knob.   

// __Hardware__

// The following parts are needed for this project:

  // 1x Adafruit IO compatible Feather
  // 1x potentiometer
  // 1x RGB LED
  // 1x OLED module
  // Some jumper wires

// **********************************************

// | Bi-directional Communication with your Adafruit IO Dashboard |
// Program for sending color data from Adafruit IO to a RGB LED.

// __Hardware__

// The following parts are needed for this project:

  // 1x Adafruit IO compatible Feather
  // 1x diffused RGB LED - common anode
  // 3x 560 ohm resistors
  // 4x jumper wires

/************************** Configuration ***********************************/

#include "config.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <ESP8266HTTPClient.h>  // Include this library, which provides methods to send HTTP requests.
#include <ArduinoJson.h>        //provides the ability to parse and construct JSON objects

// RGB LED connected to ESP8266 PWM pins
#define RED_PIN   12
#define GREEN_PIN 13
#define BLUE_PIN  14

// Potentiometer connected to analog pin 0
#define POTPIN A0

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variable to store potentiometer values 
int potValue = 0;

// set up the 'color' feed
AdafruitIO_Feed *color = io.feed("color");

typedef struct { // here we create a new data type definition, a box to hold other data types
  String names;
  String surnames;
  String region;

} RandomNames;            

RandomNames na;     //we have created a News type, but not an instance of that type,
                   //so we create the variable 'na' of type RandomNames

void setup() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  // start the serial connection
  Serial.begin(115200);

  // wait for serial monitor to open
  while(! Serial);

  // connect to io.adafruit.com
  #if defined(ARDUINO_ARCH_ESP32) // ESP32 pinMode
    // assign rgb pins to channels
    ledcAttachPin(RED_PIN, 1);
    ledcAttachPin(GREEN_PIN, 2);
    ledcAttachPin(BLUE_PIN, 3);
    // init. channels
    ledcSetup(1, 12000, 8);
    ledcSetup(2, 12000, 8);
    ledcSetup(3, 12000, 8);
  #else
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
  #endif

  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // set up a message handler for the 'color' feed.
  // the handleMessage function (defined below)
  // will be called whenever a message is
  // received from adafruit io.
  color->onMessage(handleMessage);

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
  color->get();

  // set analogWrite range for ESP8266
  #ifdef ESP8266
    analogWriteRange(255);
  #endif

}

void loop() {

  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();
  // Grab the current value of the potentiometer
  potValue = analogRead(POTPIN);
  // Cast the value as a String
  String number = String(potValue);
  // Print the value the serial monitor
  Serial.println("value is: " + number);
  delay(50);
  // If the value of the potentiometer is less than 5
  if (potValue < 100) {
    // Make a request to get a random name
    getRandomName();
    // Send a message to the serial monitor saying that the value is less than 100
    Serial.println(" -----------------------------------------");
    Serial.println("|             LESS THAN A 100             |");
    Serial.println(" -----------------------------------------");
    // Clear OLED display
    display.clearDisplay();
    // Normal 1:1 pixel scale
    display.setTextSize(1);
    // Start at top-left corner
    display.setCursor(0, 0);
    // Draw white text
    display.setTextColor(WHITE);
    // Display random name on OLED
    display.println("name: " + na.names);
    // Display random surname on OLED
    display.println("surname: " + na.surnames);
    // Display random region on OLED
    display.println("region: " + na.region);
    display.display();
    // To keep the amount of requests to a minimum, I put a long-ish delay here
    delay(10000);
  }

}

// this function is called whenever a 'color' message
// is received from Adafruit IO. it was attached to
// the color feed in the setup() function above.
void handleMessage(AdafruitIO_Data *data) {

  // print RGB values and hex value
  Serial.println("Received:");
  Serial.print("  - R: ");
  Serial.println(data->toRed());
  Serial.print("  - G: ");
  Serial.println(data->toGreen());
  Serial.print("  - B: ");
  Serial.println(data->toBlue());
  Serial.print("  - HEX: ");
  Serial.println(data->value());

  // invert RGB values for common anode LEDs
  #if defined(ARDUINO_ARCH_ESP32) // ESP32 analogWrite
    ledcWrite(1, 255 - data->toRed());
    ledcWrite(2, 255 - data->toGreen());
    ledcWrite(3, 255 - data->toBlue());
  #else
    analogWrite(RED_PIN, 255 - data->toRed());
    analogWrite(GREEN_PIN, 255 - data->toGreen());
    analogWrite(BLUE_PIN, 255 - data->toBlue());
  #endif
  
}

void getRandomName() {   // function called getRandomName that generates a random name, surname, and region using the uinames API 
  HTTPClient theClient;  // Use HttpClient object to send requests
  //Serial.println("Making HTTP request"); // print this message to the serial monitor
  theClient.begin("http://uinames.com/api/"); // API call 
  int httpCode = theClient.GET();

  if (httpCode > 0) { // if we get something back
    if (httpCode == 200) { // and it's equal to 200
      //Serial.println("Received HTTP payload."); // then print this 
      DynamicJsonBuffer jsonBuffer;//  Dynamic Json buffer is allocated on the heap and grows automaticallyis 
      // it is also the entry point for using the library: it handles the memory management and calls the parser
      String payload = theClient.getString();
      //Serial.println("Parsing..."); // print this message to the serial monitor
      JsonObject& root = jsonBuffer.parse(payload);

      // Test if parsing succeeds.
      if (!root.success()) { // If parsing doesn't successed 
        Serial.println("parseObject() failed"); // Print this to serial monitor 
        Serial.println(payload); // Print the actual information or message in transmitted data, as opposed to automatically generated metadata.
        return;
      }
      // Parse json object and get random name, surname, and region 
      na.names = root["name"].as<String>();  
      na.surnames = root["surname"].as<String>();
      na.region = root["region"].as<String>();           


    } else {
      Serial.println("Something went wrong with connecting to the endpoint."); // if we were not, for some reason, able to receive responses, then print this tp dserial monitor 
    }
  }
}
