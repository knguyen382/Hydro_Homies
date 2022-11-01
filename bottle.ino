#include <HCSR04.h>
#include <WiFi.h>

#define TRIGGER_PIN  0  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     1  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 400 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

long unsigned int Volume;
//float PI = 3.1415;
float radius = 4.5;
float height_water;
float height_sensor;

// Replace with your network credentials

const char* ssid = "abc";
const char* password = "6785702328";

// Set web server port number to 80
WiFiServer server(80);

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

//NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
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
  server.begin();
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
  web_app();
  
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
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) 
  {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Hydro Homies</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 26  
            client.println("<p>Water level: </p>");
            client.print(Volume);
            client.print(" oz. ");
            client.println("</body></html>");


            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}