#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "WebOTA.h"

const char* ssid = "<ssid>";
const char* password = "<password>";
const char* mqtt_server = "<server>";

const char* clientId = "Buzzer01";

int frequency=432; //Specified in Hz
int buzzPin=5; 
int togglePin=4;
int timeOn=1000; //specified in milliseconds
int timeOff=1000; //specified in millisecods

WiFiClient espClient;
PubSubClient client(espClient);

char timestring[50];
char msg[50];
long lastMsg = 0;
unsigned int cnt = 0;
unsigned int cmdcnt = 0;
int active=0;
int first=1;
char buf [5];

/********************************************************
/*  Handle WiFi events                                  *
/********************************************************/
void eventWiFi(WiFiEvent_t event) {
     
  switch(event) {
    case WIFI_EVENT_STAMODE_CONNECTED:
      Serial.println("EV1");
    break;
    
    case WIFI_EVENT_STAMODE_DISCONNECTED:
      Serial.println("EV2");
      ESP.restart();
    break;
    
     case WIFI_EVENT_STAMODE_AUTHMODE_CHANGE:
      Serial.println("EV3");
    break;
    
    case WIFI_EVENT_STAMODE_GOT_IP:
      Serial.println("EV4");
    break;
    
    case WIFI_EVENT_STAMODE_DHCP_TIMEOUT:
      Serial.println("EV5");
      ESP.restart();
    break;
    
    case WIFI_EVENT_SOFTAPMODE_STACONNECTED:
      Serial.println("EV6");
    break;
    
    case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED:
      Serial.println("EV7");
      ESP.restart();
    break;
    
    case WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED:
      Serial.println("EV8");
    break;
  }
}

void setup_wifi() {
  int loopcnt = 0;
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.setAutoReconnect(false);
  WiFi.onEvent(eventWiFi); 
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    loopcnt++;
    if (loopcnt > 20){
      ESP.restart();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  cmdcnt++;
  if (payload[0] == 'A') {
    Serial.println("Action triggered A");
    active=1;
    frequency=432;
    timeOn=400;
  }
  if (payload[0] == 'B') {
    Serial.println("Action triggered B");
    active=0;
  }
  if (payload[0] == 'C') {
    Serial.println("Action triggered C");
    active=1;
    frequency=432;
    timeOn=400;
  }
  if (payload[0] == 'D') {
    Serial.println("Action triggered D");
    active=1;
    frequency=128;
    timeOn=1000;
  }
  if (payload[0] == 'E') {
    Serial.println("Action triggered E");
    active=1;
    frequency=852;
    timeOn=100;
    timeOff=100;
  }
  if (payload[0] == 'F') {
    Serial.println("Action triggered F");
    active=1;
    frequency=528;
    timeOn=1000;
    timeOff=30;
  }
  strcpy(msg,clientId);
  strcat(msg,"/outTopic/cmdcount");
  sprintf (buf, "%i", cmdcnt);
  client.publish(msg, buf);
  strcpy(msg,clientId);
  strcat(msg,"/outTopic/state");
  if (active==0) strcpy(buf,"off");
  else strcpy(buf,"on");
  client.publish(msg, buf);
}

char* TSystemUptime() {
  long millisecs = millis();
  int systemUpTimeMn = int((millisecs / (1000 * 60)) % 60);
  int systemUpTimeHr = int((millisecs / (1000 * 60 * 60)) % 24);
  int systemUpTimeDy = int((millisecs / (1000 * 60 * 60 * 24)) % 365);
  sprintf(timestring,"%d days %02d:%02d:%02d", systemUpTimeDy, systemUpTimeHr, systemUpTimeMn, 0);
  return timestring;
}

void reconnect() {
  // Loop until we're reconnected
  int loopcnt = 0;
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    // Attempt to connect
    if (client.connect(clientId)) {
      loopcnt = 0;
      Serial.println("connected");
      // Once connected, publish an announcement...
      strcpy(msg,clientId);
      strcat(msg,"/outTopic/uptime");
      client.publish(msg, TSystemUptime());
      strcpy(msg,clientId);
      strcat(msg,"/outTopic/IP");
      client.publish(msg, WiFi.localIP().toString().c_str());
      if (first==1){
        strcpy(msg,clientId);
        strcat(msg,"/outTopic/state");
        strcpy(buf,"off");
        client.publish(msg, buf);
        first=0;
      }
      // ... and resubscribe
      strcpy(msg,clientId);
      strcat(msg,"/inTopic");
      client.subscribe(msg);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      loopcnt++;
      if (loopcnt > 20){
        if (WiFi.status() != WL_CONNECTED) {
          ESP.restart();
        }
      }
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(togglePin, INPUT_PULLUP);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}


void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(togglePin)==LOW && active==1){
      active=0;
      strcpy(msg,clientId);
      strcat(msg,"/outTopic/state");
      strcpy(buf,"off");
      client.publish(msg, buf);
  }
  if (active==1){
   tone(buzzPin, frequency);
   delay(timeOn);
   noTone(buzzPin);
   delay(timeOff);
  }

  char buf [5];

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  webota.handle();

  long now = millis();
  if (now - lastMsg > 300000) {
    if (WiFi.status() != WL_CONNECTED){
      ESP.restart();
    }
    lastMsg = now;
    Serial.print("Publish message: ");
    Serial.println(TSystemUptime());
    strcpy(msg,clientId);
    strcat(msg,"/outTopic/count");
    cnt++;
    sprintf (buf, "%i", cnt);
    client.publish(msg, buf);
    strcpy(msg,clientId);
    strcat(msg,"/outTopic/uptime");
    client.publish(msg, TSystemUptime());
  }
}
