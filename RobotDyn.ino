#include <RBDdimmerESP8266.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

const char* ssid = "ssid"; //type your WIFI information inside the quotes
const char* password = "wifipassword";
const char* mqtt_server = "192.168.2.123";
const char* mqtt_username = "";
const char* mqtt_password = "";
const int mqtt_port = 1883;

#define SENSORNAME "dimmer" //change this to whatever you want to call your device

#define OTApassword "123" //the password you will need to enter to upload remotely via the ArduinoIDE
int OTAport = 8266;

const char* light_state_topic = "home/dimmer";
const char* light_set_topic = "home/dimmer/set";

int dim1 = 0;// Dimming level (0-100)
int preVal = 0;

WiFiClient espClient;
PubSubClient client(espClient);
dimmerLampESP8266 dimmer(12, 14);                  // D6 for dimming, D5 for zero-cross
  
long previousMillis;

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  if (WiFi.status() != WL_CONNECTED) {
    delay(1);
    Serial.print("WIFI Disconnected. Attempting reconnection.");
    setup_wifi();
    return;
  }


  client.loop();
  ArduinoOTA.handle();
//  delay(50);
}

void sendState()
{
  if (preVal != dim1)
  {
    char strCh[10];
    String buffer = String(dimmer.getPower());
    buffer.toCharArray(strCh,9);
    client.publish(light_state_topic, strCh , true);
  }
}

void setup() {
  
  dimmer.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE) 
  pinMode(14, INPUT);
  pinMode(5, OUTPUT);

  // setup serial communication

  Serial.begin(9600);
  while (!Serial) {};
  Serial.println(F("dimmer"));
  Serial.println();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  //OTA SETUP
  ArduinoOTA.setPort(OTAport);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(SENSORNAME);

  // No authentication by default
  ArduinoOTA.setPassword((const char *)OTApassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  char msgBuffer[20];
  
   payload[length] = '\0';            // terminate string with '0'
  String strPayload = String((char*)payload);  // convert to string
  Serial.print("strPayload =  ");
  Serial.println(strPayload); //can use this if using longer southbound topics
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");//MQTT_BROKER
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println(payload[0]);

dim1=strPayload.toInt();


  dimmer.setPower(dim1); // setPower(0-100%);

  sendState();
  
  preVal = dim1;

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(light_set_topic);
      sendState();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
