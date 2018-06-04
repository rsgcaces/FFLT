// Purpose  :Fergus Fidget Logic Trainer Driver Code
//          :Grade 5 Binary Logic Unit (December 2017)
// Reference:
// Author   :C. D'Arcy
// Date     :2017 11 18 - 2018 05 06 - 2018 05 26!
// Status   :Working (except Speaker)
#define DURATION 5000
uint8_t swXOR710 = 15;
uint8_t swXOR314 = 14;
uint8_t swOR710 = 17; // mistakenly not connected in v6 :(
uint8_t swOR314 = 16;
uint8_t swAND710 = 19;
uint8_t swAND314 = 18;
uint8_t swBUF36 = 20;
uint8_t switchInputPins[] = {swXOR314, swXOR710, swOR314, swOR710, swAND314, swAND710, swBUF36}; //PB5-4,PC4-0
//Strategic use of pins: interrupts and use of embedded low-level
uint8_t ROT8 = 5;
uint8_t ROT4 = 4;
uint8_t ROT2 = 3;
uint8_t ROT1 = 2;
uint8_t BCDInputPins[] = {ROT1, ROT2, ROT4, ROT8};  //PD2-5 (PD2(Int0):New Game on Change
uint8_t BCD0 = 8;
uint8_t BCD1 = 9;
uint8_t BCD2 = 10;
uint8_t BCD3 = 11;
uint8_t BCDOutputPins[] = {BCD0, BCD1, BCD2, BCD3}; //PB0-3
uint8_t COMP0 = 0;
uint8_t COMP1 = 1;
uint8_t COMP2 = 6;
uint8_t COMP3 = 7;
uint8_t compOutputPins[] = {COMP0, COMP1, COMP2, COMP3}; //PD0-1, PD6-7
uint8_t speakerPin = 5;    //PD5
uint16_t reading, reading1, reading2, original;
uint32_t timeStamp;
uint8_t LEDs[4] = {LOW, LOW, HIGH, HIGH};
uint8_t logicGates[4];
String logicGateNames[] = {"XNOR", "XOR", "OR", "NOR", "AND", "NAND", "BUF", "NOT"};
//--------------------------------
uint8_t switches;
uint8_t compRandom;
uint8_t BCD;
volatile uint8_t value;
const uint8_t interruptPin = 2;
volatile boolean sameGame = false;
//--------------------------------
void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(BCDInputPins[i], INPUT);      //to be sure...
    pinMode(BCDOutputPins[i], OUTPUT);
    pinMode(compOutputPins[i], OUTPUT);
  }
  for (int i = 0; i < 7; i++)
    pinMode(switchInputPins[i], INPUT);   //default, but to be sure...
  pinMode(A7, INPUT);                   // determine if additional Analog pin can help with randomness
  pinMode(speakerPin, OUTPUT);          //eventually, perhaps
  // playSpeaker();
  rotary();
  attachInterrupt(digitalPinToInterrupt(interruptPin), rotary, CHANGE);
  newGame();
}

void newGame() {
  BCD = (PIND & B00111100) >> 2;      //grab the rotary setting (working:)
  setLogicGates();
  sameGame = true;
}

void setLogicGates() {
  compRandom = (millis() + analogRead(A7)) & 0x0F; //random(16);      //define them
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(compOutputPins[i], compRandom & (1 << i) );
    // design: even (0,2,4,6) + optional 1
    logicGates[i] = (i << 1) + ( compRandom & (1 << i) ? 1 : 0  ) ;
  }
}

void setBCDOutputPins() {
  for (uint8_t i = 0; i < 4; i++)
    digitalWrite(BCDOutputPins[i], LEDs[i]);
}

void getSwitches() {
  switches = 0;
  for (uint8_t i = 0; i < 7; i++) {
    switches |= analogRead(switchInputPins[i]) > 1020 ? 0 : (1 << i);
    delay(5);
  }
}

void loop() {
  setBCDOutputPins();
  while (sameGame) {
    getSwitches();
    for (uint8_t i = 0; i < 4; i++)
      switch (logicGates[i]) {
      /*XOR*/ case 0: LEDs[i] = ((switches & (1 << 1)) >> 1) ^ (switches & 1) ? HIGH : LOW; break;
      /*XNOR*/case 1: LEDs[i] = ((switches & (1 << 1)) >> 1) ^ (switches & 1) ? LOW : HIGH; break;
      /*OR*/  case 2: LEDs[i] = switches & (1 << 3) || switches & (1 << 2) ? HIGH : LOW; break;
      /*NOR*/ case 3: LEDs[i] = switches & (1 << 3) || switches & (1 << 2) ? LOW : HIGH; break;
      /*AND*/ case 4: LEDs[i] = switches & (1 << 5) && switches & (1 << 4) ? HIGH : LOW; break;
      /*NAND*/case 5: LEDs[i] = switches & (1 << 5) && switches & (1 << 4) ? LOW : HIGH; break;
      /*BUF*/ case 6: LEDs[i] = switches & (1 << 6) ? HIGH : LOW; break;
      /*NOT*/ case 7: LEDs[i] = switches & (1 << 6) ? LOW : HIGH; break;
      }
    setBCDOutputPins();
    uint8_t BCDfromLEDs = 0;
    for (int i = 0; i < 4; i++)
      BCDfromLEDs += (LEDs[i] << i);
    if (BCD == BCDfromLEDs) showWinner();
  }
  newGame();
}

void rotary() {
  sameGame = false;
}

void showWinner() {
  // press reset to stop
  while (true) {
    for (uint8_t i = 0; i < 4; i++)
      digitalWrite(compOutputPins[i], HIGH);
    delay(20);
    for (uint8_t i = 0; i < 4; i++)
      digitalWrite(compOutputPins[i], LOW);
    delay(20);
  }
}

//~~~~~~~~~~~~~~~~~~~~Code that may not be needed~~~~~~~~~~~~
void playSpeaker() {
  // CTC (Mode ?): Toggle OCOB (PD5) on Compare Match
  TCCR0A = (1 << COM0B0) | (1 << WGM01);
  TCCR0B = 0;
  OCR0A = 128;   // TOP
  TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
  interrupts();
}

ISR(TIMER1_OVF_vect) {
  TCNT1 = 32000;   // preload timer
  digitalWrite(speakerPin, digitalRead(speakerPin) ^ 1);
  // PORTB ^= (1 << PB5);
}

