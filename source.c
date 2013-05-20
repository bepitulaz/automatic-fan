/**
 * Automatic FAN
 *
 * Copyright (c) 2006-2013 Akeda Bagus <admin@gedex.web.id>
 *
 * Licensed under The MIT License
 * For full copyright and license information, please see the LICENSE
 * Redistributions of files must retain the above copyright notice.
 */

#include <mega8535.h>
#include <delay.h>
#include <stdlib.h>
#include <math.h>

// FAN
#define en_FAN1 PORTB.0
#define on_FAN1 PORTB.1 // Active low
#define en_FAN2 PORTB.2
#define on_FAN2 PORTB.3 // Active low
unsigned char pwmFAN1,pwmFAN2,x;

// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
  TCNT0 = 0xC0;

  x++;
  if ( x == 255) {
    x=0;
  }
  if ( x >= pwmFAN1 ) {
    en_FAN1 = 0;
  } else {
    en_FAN1 = 1;
  }

  if ( x >= pwmFAN2 ) {
    en_FAN2 = 0;
  } else {
    en_FAN2 = 1;
  }
}


// Alphanumeric LCD Module functions
#asm
  .equ __lcd_port=0x15
#endasm
#include <lcd.h>

void cursor_on() {
  _lcd_ready();
  _lcd_write_data(0xf);
}

void cursor_off() {
  _lcd_ready();
  _lcd_write_data(0xC);
}

typedef unsigned char byte;
flash byte _derajat[8]={   // Degree char
  0b0000110,
  0b0001001,
  0b0000110,
  0b0000000,
  0b0000000,
  0b0000000,
  0b0000000,
  0b0000000
};

/**
 * Define chararcter
 * @param byte flash
 * @param byte char_code
 */
void define_char(byte flash *pc,byte char_code)
{
  byte i,a;
  a = (char_code << 3) | 0x40;
  for ( i = 0; i < 8; i++)
    lcd_write_byte(a++,*pc++);
}

#define ADC_VREF_TYPE 0x40
// Read the AD conversion result
unsigned int read_adc(unsigned char adc_input)
{
  ADMUX = adc_input|ADC_VREF_TYPE;

  // Start the AD conversion
  ADCSRA |= 0x40;

  // Wait for the AD conversion to complete
  while ((ADCSRA & 0x10)==0);

  ADCSRA |= 0x10;
  return ADCW;
}

#define derajat 0
int  suhu,suhu_;
char strSuhu[4];
char strSuhu_[4];
int  bts_bawah,bts_atas;

void init_suhu() {
  bts_bawah = 29;
  bts_atas  = 33;
  define_char(_derajat,derajat); //simbol derajat

  TCNT0=0xc0;
  //FAN off
  en_FAN1 = 1;
  on_FAN1 = 1;
  pwmFAN1 = 0;

  en_FAN2 = 1;
  on_FAN2 = 1;
  pwmFAN2 = 0;
}

// Declare your global variables here
#define keypad PORTD
#define keypad_as DDRD
#define keypad_in PIND
#define C1 0
#define C2 1
#define C3 2
#define R1 4
#define R2 5
#define R3 6
#define R4 7

char getKeypad () {
  char dataKeypad;

  dataKeypad = 'x';
  keypad_as  = 0xF8; //C1 - C2 as input

  chkR1:
    keypad = 0xeF;
  chkR1C1:
    if ( keypad_in.0 ==1 ) {
      goto chkR1C2;
    }
    delay_ms(100);
    dataKeypad = '*';
    goto gotKeypad;
  chkR1C2:
    if ( keypad_in.1 == 1 ) {
      goto chkR1C3;
    }
    delay_ms(100);
    dataKeypad='2';
    goto gotKeypad;
  chkR1C3:
    if ( keypad_in.2 == 1 ) {
      goto chkR2;
    }
    delay_ms(100);
    dataKeypad='3';
    goto gotKeypad;
  chkR2:
    keypad = 0xd0 ; // Gives pullup
  chkR2C1:
    if ( keypad_in.C1 == 1 ) {
      goto chkR2C2;
    }
    delay_ms(100);
    dataKeypad = '1';
    goto gotKeypad;
  chkR2C2:
    if ( keypad_in.C2 == 1 ) {
      goto chkR2C3;
    }
    delay_ms(100);
    dataKeypad = '5';
    goto gotKeypad;
  chkR2C3:
    if ( keypad_in.C3 == 1 ) {
      goto chkR3;
    }
    delay_ms(100);
    dataKeypad = '6';
    goto gotKeypad;
  chkR3:
    keypad  = 0xb0 ;
  chkR3C1:
    if ( keypad_in.C1 == 1 ) {
      goto chkR3C2;
    }
    delay_ms(100);
    dataKeypad = '4';
    goto gotKeypad;
  chkR3C2:
    if ( keypad_in.C2 == 1 ) {
      goto chkR3C3;
    }
    delay_ms(100);
    dataKeypad = '8';
    goto gotKeypad;
  chkR3C3:
    if ( keypad_in.C3 == 1 ) {
      goto chkR4;
    }
    delay_ms(100);
    dataKeypad = '9';
    goto gotKeypad;
  chkR4:
    keypad = 0x70;
  chkR4C1:
    if ( keypad_in.C1 == 1 ) {
      goto chkR4C2;
    }
    delay_ms(100);
    dataKeypad = '7';
    goto gotKeypad;
  chkR4C2:
    if ( keypad_in.C2 == 1 ) {
      goto chkR4C3;
    }
    delay_ms(100);
    dataKeypad = '0';
    goto gotKeypad;
  chkR4C3:
    if ( keypad_in.C3 == 1 ) {
      goto gotKeypad;
    }
    delay_ms(100);
    dataKeypad = '#';
  gotKeypad:
    delay_ms(50);
    return dataKeypad;
}

char inSuhu[3];
void _inSuhu(unsigned char batas) {
  char i, tmp;

  ambil:
  for ( i = 0; i < 3; i++ ) {
    inSuhu[i]='x';
  }

  lcd_gotoxy(0,1);
  cursor_on();
  for ( i = 0; i < 3; i++ ) {
    _ambil:
    do {
      inSuhu[i] = getKeypad();
    } while ( inSuhu[i] == 'x');

    if ( inSuhu[i] == '*') {
      goto done;
    } else if ( inSuhu[i] == '#') {
      goto done;
    } else {
      lcd_gotoxy(i,1);
      lcd_putchar(inSuhu[i]);
    }
  }

  done:
  // Check first digit
  if ( inSuhu[0] == '*')  {
    // If no input but OK is pressed
    goto ambil;
  }
  if ( inSuhu[0] == '#' ) {
    // If no input but Cancel is pressed
    goto exit; // Upper threshold doesn't change
  }

  // Convert data
  if ( inSuhu[1] == '*' ) {
    // One digit, index 0
    if ( batas == 0) {
      tmp = bts_bawah;
      inSuhu[1] = '';
      inSuhu[2] = '';
      bts_bawah = atoi(inSuhu);
      if ( bts_bawah > bts_atas ) {
        cursor_off();
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Bts bawah mele-");
        lcd_gotoxy(0,1);
        lcd_putsf("bihi bts atas!!");
        bts_bawah = tmp;
        delay_ms(1500);

        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Input bts bawah:");
        goto ambil;
      } else {
        goto exit;
      }
    } else {
      tmp = bts_atas;
      inSuhu[1] = '';
      inSuhu[2] = '';
      bts_atas = atoi(inSuhu);
      if ( bts_atas < bts_bawah ) {
        cursor_off();
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Bts atas kurang");
        lcd_gotoxy(0,1);
        lcd_putsf("dari bts bawah!!");
        bts_atas=tmp;
        delay_ms(1500);

        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Input bts atas:");
        goto ambil;
      } else if ( bts_atas > 150 ) {
        cursor_off();
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Bts atas lebih");
        lcd_gotoxy(0,1);
        lcd_putsf("dari 150");
        lcd_putchar(derajat);
        lcd_putchar('C');
        lcd_putsf(" !!");
        bts_atas = tmp;
        delay_ms(1500);

        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Input bts atas:");
        goto ambil;
      } else {
        goto exit;
      }
    }

  } else if ( inSuhu[1] == '#' ) {
    // Back to index 0
    lcd_gotoxy(1,1);
    lcd_putchar(' ');
    lcd_gotoxy(0,1);
    lcd_putchar(' ');
    lcd_gotoxy(0,1);
    i = 0;
    goto _ambil;
  } else if ( inSuhu[2] == '*') {
    // Two digits
    if ( batas == 0 ) {
      tmp=bts_bawah;
      inSuhu[2] = '';
      bts_bawah = atoi(inSuhu);
      if ( bts_bawah > bts_atas ) {
        cursor_off();
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Bts bawah mele-");
        lcd_gotoxy(0,1);
        lcd_putsf("bihi bts atas!!");
        bts_bawah = tmp;
        delay_ms(1500);

        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Input bts bawah:");
        goto ambil;
      } else {
        goto exit;
      }
    } else {
      tmp = bts_atas;
      inSuhu[2] = '';
      bts_atas = atoi(inSuhu);
      if ( bts_atas < bts_bawah ) {
        cursor_off();
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Bts atas kurang");
        lcd_gotoxy(0,1);
        lcd_putsf("dari bts bawah!!");
        bts_atas = tmp;
        delay_ms(1500);

        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Input bts atas:");
        goto ambil;
      } else if ( bts_atas > 150 ) {
        cursor_off();
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Bts atas lebih");
        lcd_gotoxy(0,1);
        lcd_putsf("dari 150");
        lcd_putchar(derajat);
        lcd_putchar('C');
        lcd_putsf(" !!");
        bts_atas = tmp;
        delay_ms(1500);

        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Input bts atas:");
        goto ambil;
      } else {
        goto exit;
      }
    }
  } else if ( inSuhu[2] == '#' ) {
    // Back to index 1
    lcd_gotoxy(2,1);
    lcd_putchar(' ');
    lcd_gotoxy(1,1);
    lcd_putchar(' ');
    lcd_gotoxy(1,1);
    i = 1;
    goto _ambil;
  }

  exit:
  if ( i == 3 ) {
    if ( batas == 0 ) {
      tmp = bts_bawah;
      bts_bawah = atoi(inSuhu);
      if ( bts_bawah > bts_atas ) {
        cursor_off();
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Bts bawah mele-");
        lcd_gotoxy(0,1);
        lcd_putsf("bihi bts atas!!");
        bts_bawah=tmp;
        delay_ms(1500);

        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Input bts bawah:");
        goto ambil;
      }
    } else {

      tmp = bts_atas;
      bts_atas = atoi(inSuhu);
      if ( bts_atas < bts_bawah ) {
        cursor_off();
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Bts atas kurang");
        lcd_gotoxy(0,1);
        lcd_putsf("dari bts bawah!!");
        bts_atas = tmp;
        delay_ms(1500);

        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Input bts atas:");
        goto ambil;
      } else if ( bts_atas > 150 ) {
        cursor_off();
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Bts atas lebih");
        lcd_gotoxy(0,1);
        lcd_putsf("dari 150");
        lcd_putchar(derajat);
        lcd_putchar('C');
        lcd_putsf(" !!");
        bts_atas = tmp;
        delay_ms(1500);

        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Input bts atas:");
        goto ambil;
      }
    }
    delay_ms(700);
  }
}

/**
 * Input to set lower threshold
 *
 * @return void
 */
void inBatasBawah() {
  lcd_clear();
  lcd_gotoxy(0,0);
  lcd_putsf("Input bts bawah");
  lcd_gotoxy(0,1);
  lcd_putsf("* OK, # batal");
  delay_ms(1500);

  // Get input
  lcd_clear();
  lcd_gotoxy(0,0);
  lcd_putsf("Input bts bawah:");
  _inSuhu(0);
}

/**
 * Input to set upper threshold
 */
void inBatasAtas() {
  lcd_clear();
  lcd_gotoxy(0,0);
  lcd_putsf("Input bts atas");
  lcd_gotoxy(0,1);
  lcd_putsf("* OK, # batal");
  delay_ms(1500);

  // Get input
  lcd_clear();
  lcd_gotoxy(0,0);
  lcd_putsf("Input bts atas:");
  _inSuhu(1);
}

void main(void) {
  // Declare your local variables here
  char key;
  char str[3];

  // Input/Output Ports initialization
  // Port A initialization
  // Func0=In Func1=In Func2=In Func3=In Func4=In Func5=In Func6=In Func7=In
  // State0=T State1=T State2=T State3=T State4=T State5=T State6=T State7=T
  PORTA = 0x00;
  DDRA = 0x00;

  //fan
  PORTB = 0x00;
  DDRB = 0xff;

  // Port C initialization
  // Func0=In Func1=In Func2=In Func3=In Func4=In Func5=In Func6=In Func7=In
  // State0=T State1=T State2=T State3=T State4=T State5=T State6=T State7=T
  PORTC = 0x00;
  DDRC = 0x00;

  // Port D initialization
  // Func0=In Func1=In Func2=In Func3=In Func4=In Func5=In Func6=In Func7=In
  // State0=P State1=P State2=P State3=P State4=P State5=P State6=P State7=P
  PORTD = 0xFF;
  DDRD = 0x00;

  // Timer/Counter 0 initialization
  // Clock source: System Clock
  // Clock value: 62.500 kHz
  // Mode: Normal top=FFh
  // OC0 output: Disconnected
  //TCCR0=0x03;
  TCCR0 = 0x01;
  TCNT0 = 0xC0;
  OCR0 = 0x00;

  // Timer/Counter 1 initialization
  // Clock source: System Clock
  // Clock value: Timer 1 Stopped
  // Mode: Normal top=FFFFh
  // OC1A output: Discon.
  // OC1B output: Discon.
  // Noise Canceler: Off
  // Input Capture on Falling Edge
  TCCR1A = 0x00;
  TCCR1B = 0x00;
  TCNT1H = 0x00;
  TCNT1L = 0x00;
  OCR1AH = 0x00;
  OCR1AL = 0x00;
  OCR1BH = 0x00;
  OCR1BL = 0x00;

  // Timer/Counter 2 initialization
  // Clock source: System Clock
  // Clock value: Timer 2 Stopped
  // Mode: Normal top=FFh
  // OC2 output: Disconnected
  ASSR = 0x00;
  TCCR2 = 0x00;
  TCNT2 = 0x00;
  OCR2 = 0x00;

  // External Interrupt(s) initialization
  // INT0: Off
  // INT1: Off
  // INT2: Off
  MCUCR = 0x00;
  MCUCSR = 0x00;


  // Timer(s)/Counter(s) Interrupt(s) initialization
  TIMSK = 0x00;

  // Analog Comparator initialization
  // Analog Comparator: Off
  // Analog Comparator Input Capture by Timer/Counter 1: Off
  // Analog Comparator Output: Off
  ACSR = 0x80;
  SFIOR = 0x00;

  // ADC initialization
  // ADC Clock frequency: 125.000 kHz
  // ADC Voltage Reference: AVCC pin
  // ADC High Speed Mode: On
  // ADC Auto Trigger Source: None
  ADMUX = ADC_VREF_TYPE;
  ADCSRA = 0x85;
  SFIOR& = 0xEF;
  SFIOR |= 0x10;

  // LCD module initialization
  lcd_init(16);
  init_suhu();


  // Global enable interrupts
  #asm("cli")

  while (1) {
    // Show lower threshold
    lcd_gotoxy(0,0);
    itoa(bts_bawah,str);
    lcd_puts(str);
    lcd_putchar(derajat);
    lcd_putchar('C');

    // Shows upper threshold
    lcd_gotoxy(11,0);
    itoa(bts_atas,str);
    lcd_puts(str);
    lcd_putchar(derajat);
    lcd_putchar('C');

    // ADC sampling ch 0
    suhu = read_adc(0);
    suhu_ = fmod((suhu * 5),10);
    suhu = suhu * 0.5;
    itoa(suhu,strSuhu);
    itoa(suhu_,strSuhu_);

    // Shows the temperature
    lcd_gotoxy(0,1);
    lcd_puts(strSuhu);
    lcd_putchar('.');
    lcd_puts(strSuhu_);
    lcd_putchar(derajat);
    lcd_putchar('C');

    // Adjust FAN based on temperature
    if ( suhu < bts_bawah ) {
      // FAN1 & FAN 2 off
      on_FAN1 = 1;
      on_FAN2 = 1;
    } else if ( suhu > bts_bawah && suhu < bts_atas ) {
      // FAN1 ON & FAN 2 off
      pwmFAN1 = 150 + (suhu - bts_bawah);
      on_FAN1 = 0;
      on_FAN2 = 1;
    } else if ( suhu > bts_atas ) {
      //FAN1 ON & FAN 2 ON
      pwmFAN1 = 150 + ( suhu - bts_atas);
      pwmFAN2 = 150 + ( suhu - bts_atas);
      on_FAN1 = 0;
      on_FAN2 = 0;
    }

    key = getKeypad();
    if ( key == '*' ) {
      inBatasBawah();
      lcd_clear();
    } else if ( key == '#' ) {
      inBatasAtas();
      lcd_clear();
    }

  };
}
