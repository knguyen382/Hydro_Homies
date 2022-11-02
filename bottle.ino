#include <HCSR04.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define TRIGGER_PIN  0  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     1  // Arduino pin tied to echo pin on the ultrasonic sensor.

const int trigPin = 2;
const int echoPin = 3;
float duration[10], distance;
float duration_sum = 0;
float duration_avg;
int i;

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
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
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
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  if (i < sizeof(duration)/sizeof(float)){
    duration [i] = pulseIn(echoPin, HIGH);
    i++;
  }
  else{
    for (int j = 0; j < sizeof(duration)/sizeof(float); j++){
      duration_sum = duration_sum + duration[j];      
    }
    duration_avg = duration_sum/(sizeof(duration)/sizeof(float));
    distance = (duration_avg*.0343)/2;
    i = 0;
    Serial.println("");
    Serial.print("Distance: ");
    Serial.println(distance);
    duration_sum = 0;
  }
  delay(1000);
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
