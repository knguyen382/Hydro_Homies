#include <HCSR04.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define TRIGGER_PIN  0  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     1  // Arduino pin tied to echo pin on the ultrasonic sensor.

long unsigned int Volume;
//float PI = 3.1415;
float radius = 4.5;
float height_water;
float height_sensor;
float volume;
// Replace with your network credentials

const char* ssid = "TheCoffeeByHand";
const char* password = "7702328850";
const char* desired_water_intake = "input";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Desired water Intake: </title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    input1: <input type="text" name="input">
    <input type="submit" value="Submit">
  </form><br>
  </body></html>)rawliteral";

// Set web server port number to 80
AsyncWebServer server(80);

// Variable to store the HTTP request
String header;

// Assign output variables to GPIO pins
const int output26 = 34;
const int output27 = 27;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

HCSR04 sonar(TRIGGER_PIN, ECHO_PIN);


void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);

  // Set outputs to LOW
  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);
  web_app();
  // Connect to Wi-Fi network with SSID and password
  /*Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();*/
  //web_app();
}

void loop() {
  //delay(200);                     // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.

  /*height_sensor = sonar.dist();
  Serial.print("Height of sensor = ");
  Serial.print(height_sensor); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.println("cm");

  height_water = (9.5) - (height_sensor);
  Serial.print("Height of water = ");
  Serial.print(height_water); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.println("cm");

  Serial.print("Volume: ");
  Serial.print(3.14 * radius * radius * height_water);
  Serial.println("mL");*/
  volume_calculation();
  Serial.println(Volume);
  
  
}

void volume_calculation()
{
  float water_level = 0;// store level in every step
  int read_value = 0; //read sensor reading in cm
  float average_water_level;
  int water_amount_in_ounce;
  for(int i=0; i<5; i++)
  { //take five reading
      read_value = sonar.dist();
      water_level = water_level + read_value;
        
  }
  average_water_level = 9.5 - water_level/5; //find average from five reading, 17 = botole height
  water_amount_in_ounce = int(3.14 * radius * radius * water_level);
  Volume = water_amount_in_ounce;
  delay(10);
}


void web_app()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(desired_water_intake)) {
      inputMessage = request->getParam(desired_water_intake)->value();
      inputParam = desired_water_intake;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Volume = atoi( inputMessage.c_str() );
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  

  server.onNotFound(notFound);
  server.begin();
  
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
