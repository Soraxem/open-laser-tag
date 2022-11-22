/* IRremoteESP8266: IRsendDemo - demonstrates sending IR codes with IRsend.
 *
 * Version 1.1 January, 2019
 * Based on Ken Shirriff's IrsendDemo Version 0.1 July, 2009,
 * Copyright 2009 Ken Shirriff, http://arcfn.com
 *
 * An IR LED circuit *MUST* be connected to the ESP8266 on a pin
 * as specified by kIrLed below.
 *
 * TL;DR: The IR LED needs to be driven by a transistor for a good result.
 *
 * Suggested circuit:
 *     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Typical digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 */

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
