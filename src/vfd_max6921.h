#define BYTE_TO_BINARY_PATTERN " %c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte) (byte & 0x80?'1':'0'), (byte & 0x40?'1':'0'), (byte & 0x20?'1':'0'), (byte & 0x10?'1':'0'), (byte & 0x08?'1':'0'), (byte & 0x04?'1':'0'), (byte & 0x02?'1':'0'), (byte & 0x01?'1':'0') 
bool debug_digit = false;

static const char *month_abbrevs[] = {
  "Gennaio",
  "Febbraio",
  "Marzo",
  "Aprile",
  "Maggio",
  "Giugno",
  "Luglio",
  "Agosto",
  "Settembre",
  "Ottobre",
  "Novembre",
  "Dicembre",
};


/*
 * Segments are connected to the relative seg_max6921_out[] (out number of MAX6921)
 * but with "custom" order for more clean PCB layout
 * First segment connected is A to out number 11
 * 
 * The same for digit connection (useful for connection without PCB)
 * In this way, we can connect VFD pins to any out of MAX6921 and adjust via software
 */                      
enum iv18vfd_segments           {  A,  B,  C,  D,  E,  F,  G,  H};
const byte seg_max6921_out[8] = { 11, 13, 16, 17, 15, 12, 14, 18 };
 
//enum iv18vfd_digits             { 1, 2, 3, 4, 5, 6, 7, 8, 9};
const byte digit_max6921_out[9] = { 7, 0, 6, 1, 5, 2, 3, 4, 8}; 


byte blank = 0x00;

byte segmentToBit( byte seg){
  byte res = 0x00;
  bitSet(res, seg_max6921_out[seg] - 11);
  return res;
}

uint16_t digitToBit( byte digit){
  uint16_t res = 0x0000;
  bitSet(res, digit_max6921_out[digit]);
  return res;
}

const byte SEG_A = segmentToBit(A);
const byte SEG_B = segmentToBit(B);
const byte SEG_C = segmentToBit(C);
const byte SEG_D = segmentToBit(D);
const byte SEG_E = segmentToBit(E);
const byte SEG_F = segmentToBit(F);
const byte SEG_G = segmentToBit(G);
const byte SEG_H = segmentToBit(H);

#pragma warning( push, 0)

const byte vfd_font[] = {
  /* 0 - 9 */
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
  SEG_B | SEG_C,
  SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,
  SEG_B | SEG_C | SEG_F | SEG_G,
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
  SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
  SEG_A | SEG_B | SEG_C,
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
  SEG_A | SEG_B | SEG_C | SEG_F | SEG_G,
  
  /* a - z */
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,
  SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
  SEG_D | SEG_E | SEG_G,
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,
  SEG_A | SEG_B | SEG_D | SEG_E | SEG_F | SEG_G,
  SEG_A | SEG_E | SEG_F | SEG_G,
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,
  SEG_C | SEG_E | SEG_F | SEG_G,
  SEG_B | SEG_C,
  SEG_B | SEG_C | SEG_D | SEG_E,
  SEG_C | SEG_E | SEG_F | SEG_G,
  SEG_D | SEG_E | SEG_F,
  SEG_C | SEG_E | SEG_G,
  SEG_C | SEG_E | SEG_G,
  SEG_C | SEG_D | SEG_E | SEG_G,
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_G | SEG_H,
  SEG_E | SEG_G,
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
  SEG_D | SEG_E | SEG_F | SEG_G,
  SEG_C | SEG_D | SEG_E,
  SEG_C | SEG_D | SEG_E,
  SEG_A | SEG_C | SEG_D | SEG_E,
  SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,
  SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,
  SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,
};
#pragma warning( pop ) 

// Store characters to display
byte vfd_data[9];
// Store the multiplexing index 
int vfd_digit = 0;

/*
  Refresh the VFD by showing the next digit
*/
void vfd_refresh() {    
  // Load segment data and shift of 11 positions (first connected segment on output 11)
  uint32_t data = vfd_data[8-vfd_digit];
  data = data << 11;
  
  // Load digit number out position
  data |= digitToBit(vfd_digit);  
  
  if(debug_digit){
    Serial.printf("Digit: %d\n", vfd_digit);    
    Serial.print("DIN ");
    for(byte i=0; i<32; i++){       
      Serial.print(bitRead(data, i));       
      if(i==8 || i== 10 || i == 18)
      Serial.print(" ");
    }
    Serial.println();
  }

  // Send data to MAX6921 with SPI interface
  SPI.write32(data);
  digitalWrite(LOAD, HIGH);
  delayMicroseconds(10);

  
  // Next digit on next call
  vfd_digit = (++vfd_digit) % 9;  
  digitalWrite(LOAD, LOW);   
}

  
/*
  Map the given char to the correct VFD segment byte
  using the font table and some internals.
*/
byte vfd_map_char(char c) {
  switch(c){
    case 'a'...'z':
      return vfd_font[c - 'a' + 10]; 
    case 'A'...'Z':
      return vfd_font[c - 'A' + 10];
    case '0'...'9':
      return vfd_font[c - '0']; 
    case ' ':
      return 0;
    case '-':
      return SEG_G;
    case '.':
      return SEG_H ;
    case '!':
      return SEG_B | SEG_C | SEG_H;
    case '?':
      return SEG_A | SEG_B | SEG_E | SEG_G | SEG_H;
    case ',':
      return SEG_C;
    default:
      return 0; 
  } 
}

/*
  Print the given string to the VFD
*/
void vfd_set_string(char *s, int offset) {
  int slen = strlen(s);  
  for (int i = offset; i < offset + 8; i++) {
    if (i < 0 || i > slen - 1) {
      vfd_data[i - offset + 1] = 0;
    }
    else {
      vfd_data[i - offset + 1] = vfd_map_char(s[i]);
    }
  }
}


int dayIndex = -9;
uint8_t dayStrlen;
 
void vfd_scroll_string(char *str, uint32_t scrollTime) {
  static uint32_t _time;
  if((dayIndex + 9) >  dayStrlen)
    return;
  if(millis() - _time > scrollTime) {
    dayIndex = (++dayIndex) % dayStrlen;
    vfd_set_string(str, dayIndex);
    _time = millis();    
  }
}

#pragma warning( push )
#pragma warning( disable : 4101)
void test_segments_order(){  
  Serial.println();
  Serial.printf("\nA: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(SEG_A));
  Serial.printf("\nB: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(SEG_B));
  Serial.printf("\nC: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(SEG_C));
  Serial.printf("\nD: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(SEG_D));
  Serial.printf("\nE: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(SEG_E));
  Serial.printf("\nF: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(SEG_F));
  Serial.printf("\nG: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(SEG_G));
  Serial.printf("\nH: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(SEG_H));

  byte testChar;
  testChar = vfd_map_char('2');
  Serial.printf("\nTest char 2: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(testChar));  
  testChar = vfd_map_char('3');
  Serial.printf("\nTest char 3: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(testChar)); 
  testChar = vfd_map_char('5');
  Serial.printf("\nTest char 5: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(testChar));
  testChar = vfd_map_char('6');
  Serial.printf("\nTest char 6: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(testChar));
  Serial.println();
  debug_digit = true;
}
#pragma warning( pop ) 
