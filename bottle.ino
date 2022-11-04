#include <HCSR04.h>
#include <WiFi.h>

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// Sensor's trigger and echo pin
#define TRIGGER_PIN  21  
#define ECHO_PIN     19  


float water_consumed;
float water_remaining;
float daily_water_intake;

int i;
float duration[10], distance;
float duration_sum = 0;
float duration_avg;
float bottle_radius;
float bottle_height;
float water_height;
float volume;

// Replace with your network credentials

const char* ssid = "abc";
const char* password = "6785702328";
String header;


// Set web server port number to 80
WiFiServer server(80);


// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

HCSR04 sonar(TRIGGER_PIN, ECHO_PIN);


void web_app(void *pvParameters);
void wifi(void * pvParameters);
void get_volume(void *pvParameters);

void setup() {
  
  Serial.begin(115200);
  
  // RTOS TASKS 

  // web app task
  xTaskCreatePinnedToCore(
  web_app
  ,  "web_app"   
  ,  5120  
  ,  NULL
  ,  2  
  ,  NULL 
  ,  ARDUINO_RUNNING_CORE);
  
  //wifi task
  xTaskCreatePinnedToCore(
  wifi
  ,  "wifi"   
  ,  4096  
  ,  NULL
  ,  2  
  ,  NULL 
  ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
  get_volume
  ,  "volume"   
  ,  4096  
  ,  NULL
  ,  2  
  ,  NULL 
  ,  ARDUINO_RUNNING_CORE);

}

void loop() 
{
  //do nothing here, let RTOS handle its tasks
}

// web application fucntion
void web_app(void *pvParameters)
{
  (void) pvParameters;
  while(1)
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
              
              // buttons
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println(".button2 {background-color: #555555;}</style></head>");
              
              // Web Page Heading
              client.println("<body><h1>Hydro Homies</h1>");
              
              // Select your gender
              client.println("<p>Select your gender: </p>");
              client.println("<p><a href=\"/M\"><button class=\"button\">MALE</button></a></p>");
              client.print("<p><a href=\"/F\"><button class=\"button\">FEMALE</button></a></p>");

              client.println("<p>Your recommended daily water intake is: </p>");
              client.print(daily_water_intake);
              client.print(" oz. ");

              client.println("<p>Water consumed: </p>");
              client.print(distance);
              client.print(" oz. ");

              client.println("<p>Water remaining: </p>");
              client.print(water_remaining);
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
          if (currentLine.endsWith("GET /M")) {
            daily_water_intake = 104;             
          }
          if (currentLine.endsWith("GET /F")) {
            daily_water_intake = 72;               
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
    vTaskDelay(100);
  }
}

// wifi function
void wifi(void *pvParameters)
{
  (void) pvParameters;
  while(1)
  {
    //if wifi is connected, delay for 5s and move on
    if(WiFi.status() == WL_CONNECTED)
    { 
      // check status every 10s
      vTaskDelay(10000/portTICK_PERIOD_MS);
      continue;
    }
    //if not, try to connect and start server
    else
    {
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
    vTaskDelay(100);
  }
}

void get_volume(void *pvParameters)
{
  (void) pvParameters;
  while(1)
  {
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(ECHO_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    
    if (i < sizeof(duration)/sizeof(float)) { //Takes 10 Samples of Duration
      duration [i] = pulseIn(ECHO_PIN, HIGH);
      Serial.print("Duration: ");
      Serial.println(duration [i]);
      i++;
    }
    else {  //Add all elements of Duration Array
      for (int j = 0; j < sizeof(duration)/sizeof(float); j++) {
        duration_sum = duration_sum + duration[j];      
      }
      duration_avg = duration_sum/(sizeof(duration)/sizeof(float));
      distance = (duration_avg*.0343)/2;
      i = 0; //Clear i for new set of samples
      Serial.println("");
      Serial.print("Distance: ");
      Serial.println(distance);
      water height = (bottle_height - distance);
      /*volume = water_height * PI * bottle_radius^2;
      Serial.print("Current Volume: :");
      Serial.println(volume);*/
      duration_sum = 0;
    }
  }
}
