#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>


const char* ssid = "Nazwa_Wi-Fi";
const char* password = "Haslo_Wi-Fi";

#define OLED_SDA 21
#define OLED_SCL 22
Adafruit_SH1106 display(21, 22);

AsyncWebServer server(80);

AsyncEventSource events("/events");

unsigned long lastTime = 0;  
unsigned long timerDelay = 500;

#define SOUND_SPEED 0.034
const int trigPin = 5;
const int echoPin = 18;

int distance;
long duration;


void getSensorReadings(){

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);

  distance = duration * SOUND_SPEED/2;


}


void initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(500);
    }
    Serial.println(WiFi.localIP());
    digitalWrite(LED_BUILTIN, HIGH);
}

String processor(const String& var){
  getSensorReadings();
  //Serial.println(var);
  if(var == "ODLEGLOSC"){
    return String(distance);
  }
  return String();


}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html lang ="pl">
<head>
  <title>Asystent parkowania ESP32 Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" charset="utf-8">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p { font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #50B8B4; color: white; font-size: 1rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 800px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); }
    .reading { font-size: 1.4rem; }
  </style>
</head>
<body>
  <div class="topnav">
    <h1>Asystent parkowania </h1>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card">
        <p><i class="fas fa-car-side"></i><i class="fas fa-arrow-right-long-to-line"></i> Odległość od ściany (cm): </p><p><span class="reading"><span id="dist">%ODLEGLOSC%</span>  </span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('distance', function(e) {
  console.log("distance", e.data);
  document.getElementById("dist").innerHTML = e.data;
 }, false);

  source.addEventListener('red_light', function(e) {
  console.log("red_light", e.data);
  document.getElementById("kolor").innerHTML = e.data;
 }, false);
 
}
</script>
</body>
</html>)rawliteral";

void setup() {
  Serial.begin(115200);
  initWiFi();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(4, OUTPUT); 


  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  delay(500);
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);


  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);

  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }

    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    getSensorReadings();
    Serial.printf("Odleglosc = %d cm \n", distance);
    Serial.println();

    events.send("ping",NULL,millis());


    if(distance < 15) {
      events.send("Zatrzymaj się!!","distance",millis());
      digitalWrite(4, HIGH);
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 25);
      display.print("Zatrzymaj sie!!");
    }
    else if(15 <= distance && distance < 1100){
      char temp[20];
      int tempDistance = distance;
      sprintf(temp, "%d %s", tempDistance, temp);
      events.send(String(distance).c_str(),"distance",millis());
      digitalWrite(4, LOW); 
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 25);
      display.print(distance);
      display.print(" cm");
      
    }
    else if(distance >= 1100) {
      events.send("Podjedź bliżej czujnika!","distance",millis());
      digitalWrite(4, LOW);
      display.clearDisplay();
      display.setCursor(0, 25);
      display.setTextSize(1);
      display.println("Podjedz blizej");
      display.println("czujnika!");
      // events.send("red","red_light",millis());
    }
    

    //events.send(String(distance).c_str(),"distance",millis());
    
    lastTime = millis();
    display.display(); 
  }
}