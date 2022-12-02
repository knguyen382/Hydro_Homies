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
float water_consumed = 0;
float water_intake_remaining;
float daily_water_intake;

// boolean variable for things that only run at the beginning
bool start = false; 
bool selected = false;

// wifi stuff
const char* ssid = "abc";
const char* password = "6785702328";
String header;
WiFiServer server(80);

// web app stuff
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;

// real time clock stuff
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000;
const int   daylightOffset_sec = 0;

// prototype function

//main functions
void web_app(void *pvParameters);
void wifi(void * pvParameters);
void waterIntakeCalculation(void *pvParameters);

// helper function
void alarm();
double get_volume();
int get_time(char time_element);

// html stuff for page heading
const char pageHeader[] PROGMEM = R"=====(

<table border="0" width="100%" cellpadding="0" cellspacing="0" bgcolor="#663399">
  <tr>
  	 <td>
  	 	<table border="0" width="85%" cellpadding="15" cellspacing="0" align="center">
           <tr>
           	   <td>
           	   	  <font face="Open Sans" color="white" size="5">
           	       <strong>Hydro Homies</strong>
           	      </font>
           	   </td>
           </tr>
  	 	</table>
  	 </td>
  </tr>
</table>

)=====";

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
  ,  5120  
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
              client.println("<style>html { font-family: serif; display: inline-block; margin: 0px auto; text-align: center;}");
              client.print(".button { background-color: #4CAF50; border-radius: 8px; border: none; color: white; padding: 8px 22px;");
              client.print("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.print(".button2 {background-color: #555555;}</style></head>");
              
              // Web Page Heading
              client.println(pageHeader);
              
              // Select your gender
              client.print("<p>Select your gender: </p>");
              client.print("<p><a href=\"/M\"><button class=\"button\">MALE</button></a></p>");
              client.print("<p><a href=\"/F\"><button class=\"button\">FEMALE</button></a></p>");

              if (selected)
              {
                client.print("<p>Your recommended daily water intake is: </p>");
                client.print(daily_water_intake);
                client.println(" ml. ");
                

                client.print("<p>Water consumed: </p>");
                client.print(water_consumed);
                client.println(" ml. ");

                client.print("<p>Current water level: </p>");
                client.print(get_volume());
                client.println(" ml. ");

                client.print("<p>Water remaining until goal: </p>");
                client.print(daily_water_intake - water_consumed);
                client.print(" ml. ");

              }

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
            daily_water_intake = 3700;
            selected = true;             
          }
          if (currentLine.endsWith("GET /F")) {
            daily_water_intake = 2700;      
            selected = true;         
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
  delay(10);
  double volume = map(distances[0],18.5, 3.16, 0, 1000);
  //Serial.println(volume);
  delay(500);
  //Serial.println(distances[0]);
  return volume;
}

//get time function
int get_time(char time_element)
{

  // uncomment this line to enable real time clock
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  time_t now = time(0);
  tm *ltm = localtime(&now);
  switch(time_element)
  {
    case ('h'):
      return ltm->tm_hour;
      break;
    case ('m'):
      return ltm->tm_min;
      break;
    case ('s'):
      return ltm->tm_sec;
      break;
  }
  return 0;
}

void waterIntakeCalculation(void * pvParameters)
{
  (void) pvParameters;
  while(1)
  {
    vTaskDelay(15000/portTICK_PERIOD_MS);

    //get current volume
    float current_volume = get_volume(); 

    /*
    Serial.print("temp volume: ");
    Serial.println(temp_volume);
    Serial.print("current volume: ");
    Serial.println(current_volume);
    */

    // some water consumed
    if(current_volume < temp_volume)
    {
      water_consumed += temp_volume - current_volume;
      last_water_consumed_time = get_time('m');
      start = true;
    }

    /*
    Serial.print("last water consumed time :");
    Serial.println(last_water_consumed_time);
    Serial.print("hours since drinking: ");
    Serial.println(get_time('m') - last_water_consumed_time);*/

    // sound alarm if it has been ___ since last drinking time, ignoring nightime
    if(get_time('m') - last_water_consumed_time >= 2 && start && get_time('h') < 20 && get_time('h') > 7)
    {
      alarm();
    }

    // new day, reset
    if (get_time('h') == 0)
    {
      water_consumed = 0;
      start;
    }
    
    // update temporary variable
    temp_volume = current_volume; 
    vTaskDelay(15000/portTICK_PERIOD_MS);
  }
  vTaskDelay(100);
}

// sound off buzzer
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
