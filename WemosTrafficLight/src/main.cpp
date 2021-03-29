#include <Arduino.h>
#include <ESP8266WiFi.h> // Connect to Wi-Fi
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include <fsm.h>

// Current state and last event that has been raised
ENUM_STATE state = STATE_START;
ENUM_EVENT event = EVENT_STATE_EXECUTED;
unsigned int timer;

// Define the state methods => zelfe volgorde als enum LET OP!!
STATE_METHODS fsm_state_methods[STATE_NUM] = {
    {start_pre, start_loop, start_post},
    {red_pre, red_loop, red_post},
    {yellow_pre, yellow_loop, yellow_post},
    {green_pre, green_loop, green_post},
    {outoforder_pre, outoforder_loop, outoforder_post}
};

// Define your transition table here. States given in the collomn and events in de row.
//
// -------------------------------------------------------------------------------------------------------------------
//      EVENTS| BUTTON          | GREEN           | TIMER           | OUTOFORDER     | RED            | STATE_EXECUTED|
// -------------------------------------------------------------------------------------------------------------------
// START      | -               | -               | -               | -              | -              | RED     
// RED        | GREEN           | GREEN           | -               | OUTOFORDER     | -              | -
// YELLOW     | -               | -               | RED             | OUTOFORDER     | -              | -
// GREEN      | -               | -               | YELLOW          | OUTOFORDER     | -              |  -            
// OUTOFORDER | -               | -               | -               | -              | RED            |           |
// -------------------------------------------------------------------------------------------------------------------
STATE_TRANSITION fsm_state_transition[STATE_NUM][EVENT_NUM] = {
{ // STATE_START
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION},
    {STATE_RED}
  },
  { // STATE_RED
    {STATE_GREEN},
    {STATE_GREEN},
    {STATE_INVALID_TRANSITION},
    {STATE_OUTOFORDER},
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION}
  }, 
  { // STATE_YELLOW
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION},
    {STATE_RED},
    {STATE_OUTOFORDER},
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION}
  },
  { // STATE_GREEN
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION},
    {STATE_YELLOW},
    {STATE_OUTOFORDER},
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION}
  },
  { // STATE_OUTOFORDER
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION},
    {STATE_INVALID_TRANSITION},
    {STATE_RED},
    {STATE_INVALID_TRANSITION},
  }
};



String getEventName (ENUM_EVENT event) {
    switch (event) {
    case EVENT_BUTTON:
      return "EVENT_BUTTON";
    case EVENT_OUTOFORDER:
        return "EVENT_OUTOFORDER";
    case EVENT_GREEN:
        return "EVENT_GREEN";
    case EVENT_RED:
        return "EVENT_RED";
    case EVENT_TIMER:
        return "EVENT_TIMER";
    case EVENT_STATE_EXECUTED:
        return "EVENT_STATE_EXECUTED";
    default:
        return "UNKNOWN";
    }
}

void FSMInit () {
  Serial.println("FSM Init");
  if( fsm_state_methods[state].pre != NULL) {
    fsm_state_methods[state].pre();
  } 
}

void FSMRaiseEvent (ENUM_EVENT new_event) {
    ENUM_STATE new_state = fsm_state_transition[state][new_event].nextState;
    if ( new_state != STATE_INVALID_TRANSITION ) {

        // call event.post
        if( fsm_state_methods[state].post != NULL) {
            fsm_state_methods[state].post() ;
        } 
        
        // call newstate ev.pre
        if( fsm_state_methods[new_state].pre != NULL) {
            fsm_state_methods[new_state].pre() ;
        } 
        
        // Set new state and event
        state = new_state;
        event = new_event;

        Serial.println("State change: " + getEventName(event) );
    }
}

void FSMLoop () {
  if( fsm_state_methods[state].loop != NULL) {
    fsm_state_methods[state].loop();
  }

  FSMRaiseEvent(EVENT_STATE_EXECUTED);
}

WiFiClient client;
WiFiClient client2;
WiFiClientSecure clients;
HTTPClient http;
PubSubClient mqtt;

// External Events!
void mqtt_callback(char* topic, byte* pl, unsigned int length) {
  String payload = "";
  for (unsigned int i=0;i<length;i++) {
    payload += (char)pl[i];
  }
  Serial.println("MQTT EVENT: " + payload);

  // Check payload, do not expect that it is correct
  if ( payload.equals("EVENT_OUTOFORDER") ) {
    FSMRaiseEvent(EVENT_OUTOFORDER);
  }
  
  if ( payload.equals("EVENT_GREEN") ) {
    FSMRaiseEvent(EVENT_GREEN);      
  }
  
  if ( payload.equals("EVENT_RED") ) {
    FSMRaiseEvent(EVENT_RED);      
  }

  if ( payload.equals("EVENT_BUTTON") ) {
    FSMRaiseEvent(EVENT_BUTTON);      
  }

}

const int RED    = D1;
const int YELLOW = D2;
const int GREEN  = D3;
const int BUTTON = D7;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  digitalWrite(RED, HIGH);
  digitalWrite(YELLOW, HIGH);
  digitalWrite(GREEN, HIGH);

  Serial.begin(9600);
  
  // Access to Internet
  WiFi.begin("Ziggo029628", "OAUGVDEU"); // Connect with the Wi-Fi
  Serial.print("Connecting with Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());


  // No certificates
  clients.setInsecure(); // We will not create a certification store here to validate sites!

  Serial.println("MQTT SETUP!");
  mqtt.setClient(client);
  mqtt.setBufferSize(2048); // override MQTT_MAX_PACKET_SIZE
  mqtt.setCallback(mqtt_callback);
  mqtt.setServer("test.mosquitto.org", 1883);
  mqtt.connect("wemos-client-lucas");
  if ( mqtt.connected() ) {
    Serial.println("MQTT CONNECTED!");
  } else {
    Serial.println("MQTT NOT CONNECTED!");
  }

  mqtt.subscribe("wemos-lucas");
}

unsigned long loopTiming;
unsigned long loopDuration;

void loop() {
  loopTiming = millis();

  FSMLoop();

  loopDuration = millis() - loopTiming;
  String ld = "loop duration: " + String(loopDuration);
 
  // mqtt.publish("wemos-workshop-123", ld.c_str());
  if ( !mqtt.loop() ) {
    Serial.println("MQTT ERROR!");
  }

  // Serial.println(ld);

  if ( loopDuration < 100 ) { // Make sure the loop is around 100 ms always!
    delay(100 - loopDuration);
  }
}

void start_pre() {
    Serial.println("start_pre");
}

void start_loop() {
    //Serial.println("start_loop");
}

void start_post() {
    Serial.println("start_post");
}

int buttonPressed;
void red_pre() {
    buttonPressed = false;
    Serial.println("red_pre");
    digitalWrite(RED, LOW);
}

void red_loop() {

  if ( !buttonPressed && digitalRead(D7) == LOW ) {
    Serial.println("BUTTON pressed");
    FSMRaiseEvent(EVENT_BUTTON);
    buttonPressed = true;
  }
}

void red_post() {
    Serial.println("red_post");
    digitalWrite(D1, HIGH);
    buttonPressed = false;
}

void yellow_pre() {
    Serial.println("yellow_pre");
    timer = millis();
    digitalWrite(D2, LOW);
}

void yellow_loop() {
    //Serial.println("yellow_loop");
    if( (millis() - timer) > 2000 ) {
      FSMRaiseEvent(EVENT_TIMER);
    }
}

void yellow_post() {
    Serial.println("yellow_post");
    digitalWrite(YELLOW, HIGH);
}

void green_pre() {
    Serial.println("green_pre");
    timer = millis();
    digitalWrite(GREEN, LOW);
}

void green_loop() {
    if ( (millis() - timer) > 5000 ) {
      FSMRaiseEvent(EVENT_TIMER);
    }
}

void green_post() {
    Serial.println("green_post");
    digitalWrite(GREEN, HIGH);
}

void outoforder_pre() {
    Serial.println("outoforder_pre");
    timer = millis();
}

void outoforder_loop() {
    if ( millis() - timer > 1000 ) { // Knipperen oranje led
      digitalWrite(YELLOW, !digitalRead(D2));
      timer = millis();
    }          
}

void outoforder_post() {
    Serial.println("outoforder_post");
    digitalWrite(YELLOW, HIGH);
}