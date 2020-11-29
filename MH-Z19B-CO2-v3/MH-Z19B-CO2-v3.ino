// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <MHZ.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>



// user variables
const long interval = 10000;    // Updates DHT readings every 10 seconds
const int co2_sensor_warmup_time = 100;
const double temp_calib_offset = 1.5; // offset, direct sensor output compensation

// acces point (wifi) configuration
char ssid[]     =  "sensor";
char password[] =  "metenisweten";

// Pin declaration of all sensors NB: SD-card uses D5, D6, D7 AND D8 (SPI pins)
// setup dht11 temp+humidity
#define DHTPIN D1     // Digital pin connected to the DHT sensor
#define DHTTYPE    DHT11     // DHT 11

// neopixel WS2812b
#define PIN D2   // LED output pin
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 1
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800);

// co2 sensor
#define CO2_IN D3   // pwm - D3 change to D8 for upload problems?
// pin for uart reading
#define MH_Z19_RX D4  // D7
#define MH_Z19_TX D0  // D6


// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;
int ppm = 0;
int maxppm = 0;

unsigned long startTime;

// initialize sensor classes
MHZ co2(MH_Z19_RX, MH_Z19_TX, CO2_IN, MHZ19B);
DHT dht(DHTPIN, DHTTYPE);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>CO2 sensor</h2>
  <p>
    <span class="dht-labels">CO2</span> 
    <span id="ppm">%PPM%</span>
    <sup class="units">ppm</sup>
  </p>
    <p>
    <span class="dht-labels">Maximum CO2</span> 
    <span id="maxppm">%MAXPPM%</span>
    <sup class="units">ppm</sup>
  </p>
  <p>
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("ppm").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/ppm", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("maxppm").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/maxppm", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var) {
  //Serial.println(var);
  if (var == "TEMPERATURE") {
    return String(int(round(t)));
  }
  else if (var == "HUMIDITY") {
    return String(int(h));
  }
  else if (var == "PPM") {
    return String(ppm);
  }
  else if (var == "MAXPPM") {
    return String(maxppm);
  }
  return String();
}

void NeoBlink(int brightness, int wait)
{
  pixels.setPixelColor(0, 0, 0, 0);
  pixels.show();
  delay(wait);
  pixels.setPixelColor(0, 0, brightness, 0);
  pixels.show();
  delay(wait);
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(9600);
  dht.begin();
  pinMode(CO2_IN, INPUT);

  // debug on
  co2.setDebug(true);
  // initalize RGB LED driver
  pixels.begin();

  // track start time
  startTime = millis();

  // Remove the password parameter, if you want the AP (Access Point) to be open
  boolean result = WiFi.softAP(ssid, password);
  if (result == true)
  {
    Serial.println("Access point Ready");
  }
  else
  {
    Serial.println("Failed!");
  }

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/ppm", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(ppm).c_str());
  });
  server.on("/maxppm", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(maxppm).c_str());
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(int(t)).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(int(h)).c_str());
  });


  // Start server
  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {

    // save the last time you updated the DHT values
    previousMillis = currentMillis;

    int ppm_uart = co2.readCO2UART();


    unsigned long secondsRunning = (millis() - startTime) / 1000;
    if (ppm_uart > 0 && secondsRunning > co2_sensor_warmup_time) {
      ppm = ppm_uart;
      Serial.print("PPMuart: ");
      Serial.println(ppm_uart);
      if (ppm_uart > 1200 )
      {
        pixels.setPixelColor(0, pixels.Color(0, 0, 180)); // Blue, warning signal
      }
      else
      {
        pixels.setPixelColor(0, pixels.Color(0, 180, 0)); // Green
      }

      if (ppm_uart > maxppm)
      {
        maxppm = ppm_uart;
      }
      Serial.print(int(secondsRunning / 60));
      Serial.println(" minutes");
      pixels.show();
    }
    else if (secondsRunning < 180)
    {
      while (millis() - currentMillis < (interval - 100))
      {
        NeoBlink(120, 500); // brightness (max 255)  , wait time (millisecond)
      }
    }
    else {
      Serial.print("n/a");
    }

    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature() - temp_calib_offset;  // correct sensor drift
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read temperature from DHT sensor!");
    }
    else {
      t = newT;
      Serial.print("Temperature: ");
      Serial.println(t);
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value
    if (isnan(newH)) {
      Serial.println("Failed to read humidity from DHT sensor!");
    }
    else {
      h = newH;
      Serial.print("Humidity: ");
      Serial.println(h);
    }
  }
}
