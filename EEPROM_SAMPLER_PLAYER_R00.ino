
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

//SHIFT REGISTER CONNECTION
#define SRCLK 4 // D4 Clock pin
#define RCLK 5 //D5 Latch pin
#define SER 3 //D3

//EEPROM CONNECTION
#define E_D0 8//D8
#define E_D1 7 //D7
#define E_D2 6 //D6
#define E_D3 9//D9
#define E_D4 10//D10
#define E_D5 12//D12
#define E_D6 13 //D13
#define E_D7 A7 //A7

#define WE A4
#define OE A5
#define CS A6

//AUDUINO CONNECTION/PINOUT
//#define ctrl_1 A0
//#define ctrl_2 A1
//#define ctrl_3 A2
//#define ctrl_4 A3
#define audio_out 11//D11

//From Auduino
// Map Analogue channels
#define TRIGGER (3)
#define FREQUENCY (1)
#define BITRATE (0)
#define SAMPLE_SELECT (2)

//From Auduino
// Changing these will also requires rewriting audioOn()
//    Output is on pin 11
#define PWM_PIN       11
#define PWM_VALUE     OCR2A
#define LED_PIN       13
#define LED_PORT      PORTB
#define LED_BIT       5
#define PWM_INTERRUPT TIMER2_OVF_vect

//From Auduino
uint16_t syncPhaseAcc;
uint16_t syncPhaseInc;
uint16_t sampleStep;

uint8_t selectedSample;
uint16_t output;
bool play;

boolean clockHigh;
boolean previousClockHigh;
uint8_t trigger;
uint8_t bitmask;

//DATA IN/OUT FROM EEPROM
int EEP_C[8] = {E_D0, E_D1, E_D2, E_D3, E_D4, E_D5, E_D6, E_D7};

//white_noise sample
const uint8_t white_noise[200] = {130, 149, 76, 190, 156, 183, 157, 134, 134, 105, 157, 161, 66, 102, 133, 110, 104, 142, 137, 148, 102, 166, 100, 134, 71, 156, 95, 103, 94, 151, 120, 185, 119, 103, 77, 158, 122, 187, 105, 134, 159, 143, 167, 150, 144, 80, 183, 172, 177, 164, 132, 185, 88, 191, 128, 107, 175, 88, 161, 74, 154, 141, 138, 83, 64, 68, 160, 179, 171, 111, 168, 159, 145, 181, 138, 117, 184, 95, 104, 168, 141, 150, 171, 152, 97, 74, 152, 96, 95, 90, 185, 180, 171, 70, 142, 97, 73, 135, 79, 134, 181, 97, 80, 143, 71, 65, 180, 153, 76, 121, 132, 65, 130, 115, 112, 123, 66, 168, 172, 131, 153, 123, 189, 129, 157, 188, 185, 101, 177, 115, 181, 164, 147, 68, 109, 86, 91, 126, 100, 96, 85, 170, 137, 152, 74, 175, 154, 175, 175, 93, 80, 184, 84, 119, 151, 145, 114, 139, 85, 161, 75, 172, 130, 156, 177, 109, 155, 187, 159, 153, 176, 170, 76, 159, 139, 148, 81, 160, 66, 137, 79, 144, 173, 166, 186, 110, 135, 71, 159, 116, 191, 160, 159, 189, 185, 132, 114, 85, 142, 69};


//From Auduino
void audioOn() {
  // Set up PWM to 31.25kHz, phase accurate

  TCCR2A = _BV(COM2A1) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  OCR2A = 15;
  TIMSK2 = _BV(TOIE2);
}
const uint16_t freqTable[] = {
549, 552, 556, 559, 563, 566, 570, 574, 577, 581, 585, 588, 592, 596, 600, 603, 607, 611, 615, 619, 623, 627, 631, 635, 639, 643, 647, 651, 655, 659, 663, 668, 672, 676, 680, 685, 689, 693, 698, 702, 707, 711, 716, 720, 725, 729, 734, 739, 743, 748, 753, 758, 763, 767, 772, 777, 782, 787, 792, 797, 802, 807, 812, 818, 823, 828, 833, 838, 844, 849, 855, 860, 865, 871, 876, 882, 888, 893, 899, 905, 910, 916, 922, 928, 934, 940, 946, 952, 958, 964, 970, 976, 982, 989, 995, 1001, 1007, 1014, 1020, 1027, 1033, 1040, 1046, 1053, 1060, 1067, 1073, 1080, 1087, 1094, 1101, 1108, 1115, 1122, 1129, 1136, 1143, 1151, 1158, 1165, 1173, 1180, 1188, 1195, 1203, 1210, 1218, 1226, 1234, 1242, 1249, 1257, 1265, 1273, 1281, 1290, 1298, 1306, 1314, 1323, 1331, 1339, 1348, 1357, 1365, 1374, 1383, 1391, 1400, 1409, 1418, 1427, 1436, 1445, 1454, 1464, 1473, 1482, 1492, 1501, 1511, 1520, 1530, 1540, 1549, 1559, 1569, 1579, 1589, 1599, 1609, 1620, 1630, 1640, 1651, 1661, 1672, 1682, 1693, 1704, 1715, 1725, 1736, 1747, 1759, 1770, 1781, 1792, 1804, 1815, 1827, 1838, 1850, 1862, 1873, 1885, 1897, 1909, 1921, 1934, 1946, 1958, 1971, 1983, 1996, 2009, 2021, 2034, 2047, 2060, 2073, 2086, 2100, 2113, 2126, 2140, 2153, 2167, 2181, 2195, 2209, 2223, 2237, 2251, 2265, 2280, 2294, 2309, 2323, 2338, 2353, 2368, 2383, 2398, 2413, 2429, 2444, 2460, 2475, 2491, 2507, 2523, 2539, 2555, 2571, 2587, 2604, 2620, 2637, 2654, 2671, 2687, 2705, 2722, 2739, 2756, 2774, 2791, 2809, 2827, 2845, 2863, 2881, 2900, 2918, 2937, 2955, 2974, 2993, 3012, 3031, 3050, 3070, 3089, 3109, 3128, 3148, 3168, 3188, 3209, 3229, 3249, 3270, 3291, 3312, 3333, 3354, 3375, 3397, 3418, 3440, 3462, 3484, 3506, 3528, 3551, 3573, 3596, 3619, 3642, 3665, 3688, 3711, 3735, 3759, 3783, 3807, 3831, 3855, 3880, 3904, 3929, 3954, 3979, 4004, 4030, 4055, 4081, 4107, 4133, 4159, 4186, 4212, 4239, 4266, 4293, 4320, 4348, 4375, 4403, 4431, 4459, 4488, 4516, 4545, 4574, 4603, 4632, 4661, 4691, 4721, 4751, 4781, 4811, 4842, 4873, 4904, 4935, 4966, 4998, 5029, 5061, 5093, 5126, 5158, 5191, 5224, 5257, 5291, 5324, 5358, 5392, 5426, 5461, 5495, 5530, 5565, 5601, 5636, 5672, 5708, 5744, 5781, 5818, 5854, 5892, 5929, 5967, 6005, 6043, 6081, 6120, 6159, 6198, 6237, 6277, 6316, 6357, 6397, 6438, 6478, 6520, 6561, 6603, 6645, 6687, 6729, 6772, 6815, 6858, 6902, 6946, 6990, 7034, 7079, 7124, 7169, 7215, 7260, 7306, 7353, 7400, 7447, 7494, 7541, 7589, 7637, 7686, 7735, 7784, 7833, 7883, 7933, 7983, 8034, 8085, 8137, 8188, 8240, 8293, 8345, 8398, 8452, 8505, 8559, 8614, 8668, 8723, 8779, 8834, 8891, 8947, 9004, 9061, 9119, 9176, 9235, 9293, 9352, 9412, 9472, 9532, 9592, 9653, 9714, 9776, 9838, 9901, 9964, 10027, 10090, 10155, 10219, 10284, 10349, 10415, 10481, 10548, 10615, 10682, 10750, 10818, 10887, 10956, 11026, 11096, 11166, 11237, 11308, 11380, 11452, 11525, 11598, 11672, 11746, 11821, 11896, 11971, 12047, 12124, 12201, 12278, 12356, 12435, 12514, 12593, 12673, 12753, 12834, 12916, 12998, 13081, 13164, 13247, 13331, 13416, 13501, 13587, 13673, 13760, 13847, 13935, 
};

const uint16_t sampleLength = 256;
void setup() {

  //SHIFT REGISTER
  pinMode(SRCLK, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SER, OUTPUT);

  //SHIFT REGISTER
  pinMode(SRCLK, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SER, OUTPUT);

  //EEPROM CONNECTION
  for (int i = 0; i < 8; i++) {
    pinMode(EEP_C[i], OUTPUT);
  }

  pinMode(WE, OUTPUT);
  pinMode(OE, OUTPUT);
  pinMode(CS, OUTPUT);

  //AUDUINO CONNECTION/PINOUT
//  pinMode(ctrl_1, INPUT);
//  pinMode(ctrl_2, INPUT);
//  pinMode(ctrl_3, INPUT);
//  pinMode(ctrl_4, INPUT);
//  pinMode(audio_out, OUTPUT);

  for (int i = 0; i < sizeof(white_noise); i++) {
    write_data(white_noise[i], i);
  }

  //From Auduino
  bitmask = 0;
  sampleStep = 0;
  play = false;
  clockHigh = false;
  previousClockHigh = false;

}

void loop() {
  syncPhaseInc = (freqTable[(analogRead(FREQUENCY)) >> 1] << 2);

  selectedSample = analogRead(SAMPLE_SELECT) >> 6;

  // Get triggers and rate values
  trigger = analogRead(TRIGGER);

  // Going from low to high, ie incoming clock pulse
  if ((trigger > 128) && !clockHigh)
  {
    clockHigh = true;
    sampleStep = 0;
    play = true;

  }

  // Going from high to low, end of incoming clock pulse
  if ((trigger <= 128) && clockHigh)
  {
    clockHigh = false;
  }
  //bitmask = ~(255-((analogRead(BITRATE)) >> 2)) >> 3;

  bitmask = ~((analogRead(BITRATE)) >> 3);


}


//This function updates the address present at the pin of the shift register
//The MSB goes first
void updateShiftRegister(int address) {
  digitalWrite(RCLK, LOW);
  shiftOut(SER, SRCLK, MSBFIRST , address);
  digitalWrite(RCLK, HIGH);
}


// **************************************************
// Write value to data location (address9
// **************************************************

void write_data(int value, int address) {
  // Configure the control pins
  digitalWrite(OE, HIGH);
  digitalWrite(WE, HIGH);
  digitalWrite(CS, LOW);
  outputToIntPins(value);
  // Set the address pins
  updateShiftRegister(address);
  digitalWrite(WE, LOW);
  delay(0.01); //10000ns
  digitalWrite(WE, HIGH);
  digitalWrite(CS, HIGH);
}

void outputToIntPins(int value) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(EEP_C[i], (value >> i) & 0x01);
  }
}


// **************************************************
// Read data from data location (address)
// **************************************************

byte read_data(int address) {
  // Configure the control pins
  digitalWrite(OE, HIGH);
  digitalWrite(WE, HIGH);
  digitalWrite(CS, LOW);

  // Configure data as input
  //EEPROM CONNECTION
  for (int i = 0; i < 8; i++) {
    pinMode(EEP_C[i], INPUT);
  }

  // Set the address pins
  updateShiftRegister(address);

  // Perform the read cycle
  digitalWrite(OE, LOW);
  delay(0.01);
  uint8_t value = 0;
  value |= digitalRead(E_D0); // Set bit 0 if E_D0 is HIGH
  value |= digitalRead(E_D1) << 1; // Set bit 1 if E_D1 is HIGH
  value |= digitalRead(E_D2) << 2; // Set bit 2 if E_D2 is HIGH
  value |= digitalRead(E_D3) << 3; // Set bit 3 if E_D3 is HIGH
  value |= digitalRead(E_D4) << 4; // Set bit 4 if E_D4 is HIGH
  value |= digitalRead(E_D5) << 5; // Set bit 5 if E_D5 is HIGH
  value |= digitalRead(E_D6) << 6; // Set bit 6 if E_D6 is HIGH
  value |= digitalRead(E_D7) << 7; // Set bit 7 if E_D7 is HIGH
  digitalWrite(OE, HIGH);
  digitalWrite(CS, HIGH);
  return value;
}

SIGNAL(PWM_INTERRUPT)
{
byte value=0;
  syncPhaseAcc += syncPhaseInc;
  if ((syncPhaseAcc < syncPhaseInc) && play) {
    // Time to increase the sample step
    if ( ++sampleStep >= sampleLength) {
      play = false;
      output = 128;
    }
    else {
     
      output = read_data(sampleStep);
      output ^= bitmask;
    }
  }
  // Output to PWM (this is faster than using analogWrite)
  PWM_VALUE = output;
}
