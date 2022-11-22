// Library integration
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

// Pin definition
#define sender D2
#define reciever D5
#define trigger D7
#define indicator D8

// Init infrared communication
IRsend irsend(sender);
IRrecv irrecv(reciever);
decode_results results;

// Attributes definition temporary
#define player 0b0000001
#define team 0b00
#define damage 0b1101

// Attributes definition
#define deathtime 20000
#define reloadtime 4000

// Init variables
bool dead = 0;
unsigned long int deathstamp = 0;
bool reload = 0;
unsigned long int reloadstamp = 0;

int offender_player;
int offender_team;
int offender_damage;

bool clock2hz = 0;

void setup() {
  // Init Pins
  irsend.begin(); // Start the sender
  irrecv.enableIRIn();  // Start the receiver

  pinMode(trigger, INPUT);
  pinMode(indicator, OUTPUT);

  // Init serial communication
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  while (!Serial)
    delay(50);
  Serial.println();
  Serial.println("Openlasertag is now running");
}


void loop() {
  clock2hz = (millis() / 250) % 2;

  // shoot if trigger is pulled
  if (not(dead) and not(reload) and digitalRead(trigger)) {
    send_shot(player, team, damage);
    reload = 1;
    reloadstamp = millis() + reloadtime;
  }

  // ending reload after delay
  if (reload and (millis() >= reloadstamp)) {
     reload = 0;
  }

  // recieving a packet
  if (irrecv.decode(&results)) {
    if (decode_shot(results.value, &offender_player, &offender_team, &offender_damage) and not(dead)){
      Serial.println("Got hit by");
      Serial.println("Player: " + String(offender_player, BIN));
      Serial.println("Team: " + String(offender_team, BIN));
      Serial.println("Damage: " + String(offender_damage, BIN));
      if (not(offender_player == player)) {
        dead = 1;
        deathstamp = millis() + deathtime;
      }
    }
    irrecv.resume();  // Receive the next value
  }

  // ending death after delay
  if (dead and (millis() >=deathstamp)) {
    dead = 0;
  }

  if (dead) {
    digitalWrite(D8, clock2hz);
  }
}

//Send a Shot. This sends a infrared packet with all lethal information
// 0   XXXXXXX   XX   XXXX
void send_shot(int player_id, int team_id, int damage_id) {
  int data = player_id;
  data = data << 2;
  data =+ team_id;
  data = data << 4;
  data =+ damage_id;
  irsend.sendGeneric(2400, 600, 1200, 600, 600, 600, 0, 0, data, 14, 56, true, 0, 85);
}

//Decode a Shot. This decodes a infrared packet with all lethal information
// 0   XXXXXXX   XX   XXXX
bool decode_shot(int data ,int *player_id, int *team_id, int *damage_id) {
  if (not(data >= 0b10000000000000)) {
    *damage_id = data & 0b1111;
    data = data >> 4;
    *team_id = data & 0b11;
    data = data >> 2;
    *player_id = data;
    return true;
  } else {
    return false;
  }
}
