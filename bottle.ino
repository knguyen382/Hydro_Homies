#include <HCSR04.h>
#include <WiFi.h>
#include "time.h"

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// Sensor's trigger and echo pin
#define TRIGGER  19  
#define ECHO    21  
#define BUZZER 13

int last_water_consumed_time;

float temp_volume = 0;
float water_consumed[] = 0;
float water_intake_remaining = 2000;
float daily_water_intake = 0;
int water_intake_times = 0;
int seconds, minutes, hours;
int idle_time = 0;
float total_water_intake_today = 0;

// wifi stuff
const char* ssid = "abc";
const char* password = "6785702328";
String header;
WiFiServer server(80);

// web app stuff
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;


int get_time(char time_element);
void web_app(void *pvParameters);
void wifi(void * pvParameters);
double get_volume();
void waterIntakeCalculation(void *pvParameters);
void alarm();

void setup() {

  HCSR04.begin(TRIGGER, ECHO);
  pinMode(BUZZER, OUTPUT);
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
  ,  3 
  ,  NULL 
  ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
  waterIntakeCalculation
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
              client.print(" ml. ");

              client.println("<p>Water consumed: </p>");
              client.print(water_consumed);
              client.print(" ml. ");

              client.println("<p>Current water level: </p>");
              client.print(get_volume());
              client.print(" ml. ");

              client.println("<p>Water remaining til goal: </p>");
              client.print(water_intake_remaining);
              client.print(" ml. ");
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
    //if wifi is connected, delay task for 30s
    if(WiFi.status() == WL_CONNECTED)
    { 
      // check status every 30s
      vTaskDelay(30000/portTICK_PERIOD_MS);
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

double get_volume()
{
  double* distances = HCSR04.measureDistanceCm();
  double volume = map(distances[0],18.5, 3.16, 0, 1000);
  Serial.println(volume);
  delay(1000);
  Serial.println(distances[0]);

  return volume;
}


void read_time() {
 tmElements_t tm;

 if (RTC.read(tm)) {
 seconds = tm.Second;
 minutes = tm.Minute;
 hours = tm.Hour;
  } 
}

void waterIntakeCalculation(void * pvParameters)
{
  (void) pvParameters;
  float current_volume = get_volume();
  if (water_intake_times == 0)
  {
    temp_volume = current_volume;
    water_intake_times = 1;
  }
  // consumed water
  if((current_volume < temp_volume) && (hours < 24))
  {
    water_consumed[water_intake_times - 1] = current_volume - temp_volume;
    water_intake_times++;
    temp_volume = current_volume;
    idle_time = 0;
  }
  // water is refilled
  else if (current_volume > temp_volume)
  {
    temp_volume = current_volume;
  }
  // no water is consumed
  else if (current_volume == temp_volume)
  {
    idle_time+=1;
  }
}

float total_water_intake_in_day()
{
    total_water_intake_today = 0;
    for (int i=0; i < water_intake_times; i++)
    {
        total_water_intake_today = total_water_intake_today + water_consumed[i];
    }
    return total_water_intake_today;
}

void alarm()
{
  for (int i = 0; i < 5; i ++)
  {  
    tone(BUZZER, 1000);
    delay(750);
    noTone(BUZZER);
    delay(750); 
  }
}
