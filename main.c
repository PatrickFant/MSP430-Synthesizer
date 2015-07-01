#include <msp430g2553.h>

// Define half-periods for the notes provided.
#define E4 1515
#define G  1276
#define A  1136
#define B  1012
#define D  852
#define E5 759

// Define bit masks for the pushbuttons.
#define mode_select BIT1
#define buttonE4    BIT2
#define buttonG     BIT3
#define buttonA     BIT4
#define buttonB     BIT5
#define buttonD     BIT6
#define buttonE5    BIT7

// Set up the potentiometer for ADC conversion.
#define ADC_INPUT BIT0
#define ADC_INCH  INCH_0

// Define a bit mask for the output channel to the speaker.
#define SPEAKER BIT1


// An array to hold the notes being pressed by the user.
volatile unsigned int notes[6] = { 0 };
// Stores the index of the next note to be played by the arpeggiator.
volatile unsigned int currentNote = 0;
// Stores the number of notes being simultaneously pressed by the user.
volatile unsigned int numberOfNotes;

// Stores the tempo of the arpeggiator (NOT in bpm).
volatile unsigned int tempo = 8;
// A counter to regulate the time in between notes in arpeggiator mode.
volatile unsigned int downCounter = 8;
// Stores the rate of pitch shift in glissando mode.
volatile unsigned int rate = 0;

// Stores the current mode of the synth.
volatile unsigned int mode = 0;


// Sets up WDT as an interval timer and enables interrupts.
void initializeWdt(void);
// Sets up Timer A to generate a square wave.
void initializeTimer(void);
// Sets up ADC10 to convert from the potentiometer.
void initializeAdc(void);
// Sets up the external pushbuttons to act as keys.
void initializeButtons(void);


int main(void)
{
  // Stop WDT.
  WDTCTL  = WDTPW + WDTHOLD;
  
  // Calibrate the clock for 8MHz operation.
  BCSCTL1 = CALBC1_8MHZ;
  DCOCTL  = CALDCO_8MHZ;

  // Initialize the peripherals.
  initializeWdt();
  initializeTimer();
  initializeAdc();
  initializeButtons();

  // Set up the speaker as output.
  P1SEL |= SPEAKER;
  P1DIR |= SPEAKER;

  // Turn off the CPU and enable interrupts.
  _bis_SR_register(GIE + LPM0_bits);

  return 0;
}

/**
 * Sets up WDT as an interval timer and enables interrupts.
 */
void initializeWdt(void)
{
  WDTCTL  = WDTPW + WDTTMSEL + WDTCNTCL;
  IE1    |= WDTIE;
}

/**
 * Sets up TA0 in up mode and divides SMCLK by 8.
 */
void initializeTimer(void)
{
  TA0CTL   |= TACLR;
  TA0CTL    = TASSEL_2 + ID_3 + MC_1;
  TA0CCTL0  = OUTMOD_4;
  TA0CCR0   = 0;
}

/**
 * Sets up ADC10 for single-channel single conversion.
 */
void initializeAdc()
{
  ADC10CTL1 = ADC_INCH | SHS_0 | ADC10DIV_4 | ADC10SSEL_0 | CONSEQ_0;
  ADC10AE0  = ADC_INPUT;
  ADC10CTL0 = SREF_0 | ADC10SHT_3 | ADC10ON | ADC10IE | ENC;
}


void initializeButtons(void)
{
  P1DIR &= ~(buttonE4 + buttonG + buttonA + buttonB + buttonD + buttonE5);
  P1REN |=  (buttonE4 + buttonG + buttonA + buttonB + buttonD + buttonE5);
  P1IN  |=  (buttonE4 + buttonG + buttonA + buttonB + buttonD + buttonE5);
  P1OUT |=  (buttonE4 + buttonG + buttonA + buttonB + buttonD + buttonE5);
  P1SEL &= ~(buttonE4 + buttonG + buttonA + buttonB + buttonD + buttonE5);

  P2DIR &= (~mode_select);
  P2REN |= (mode_select);
  P2IN  |= (mode_select);
  P2OUT |= (mode_select);
  P2SEL &= (~mode_select);
  P2SEL &= P2SEL2;
  P2IE  |= mode_select;
  P2IFG &= (~mode_select);
}


interrupt void wdtIntervalHandler()
{
  // Start ADC conversion.
  ADC10CTL0 |= ADC10SC;
  numberOfNotes = 0;

  // Clear the array.
  unsigned int i = 6;
  while (i != 0) { notes[--i] = 0; }

  // Get new values for the array.
  if (!(P1IN & buttonE4)) {
    notes[numberOfNotes] = E4;
    ++numberOfNotes;
  }
  if (!(P1IN & buttonG)) {
    notes[numberOfNotes] = G;
    ++numberOfNotes;
  }
  if (!(P1IN & buttonA)) {
    notes[numberOfNotes] = A;
    ++numberOfNotes;
  }
  if (!(P1IN & buttonB)) {
    notes[numberOfNotes] = B;
    ++numberOfNotes;
  }
  if (!(P1IN & buttonD)) {
    notes[numberOfNotes] = D;
    ++numberOfNotes;
  }
  if (!(P1IN & buttonE5)) {
    notes[numberOfNotes] = E5;
    ++numberOfNotes;
  }

  // GLISSANDO MODE:
  if (mode == 0)
  {
	  if (numberOfNotes != 0)
	  {
		  if      (TA0CCR0 < notes[numberOfNotes - 1]) {
			  TA0CCR0 += rate;
		  }
		  else if (TA0CCR0 > notes[numberOfNotes - 1]) {
			  TA0CCR0 -= rate;
		  }
	  }
  }
  // ARPEGGIATOR MODE:
  else if (mode == 1)
  {
	  if (downCounter-- == 0)
	  {
		  downCounter = tempo;

		  if (numberOfNotes != 0) {
			  TA0CCR0     = notes[currentNote];
			  currentNote = (1 + currentNote) % numberOfNotes;
		  }
		  else {
			  TA0CCR0 = 0;
		  }
	  }
  }

}
ISR_VECTOR(wdtIntervalHandler, ".int10")

/**
 * Gives the result of conversion to the rate and tempo variables.
 */
interrupt void adcHandler()
{
  rate  = ADC10MEM / 64;
  tempo = (ADC10MEM / 10) + 12;
}
ISR_VECTOR(adcHandler, ".int05")

/**
 * Handles mode switching.
 */
interrupt void buttonHandler()
{
	if (P2IFG & mode_select){
		if (mode == 0){
			mode = 1;
		}
		else if (mode == 1){
			mode = 0;
		}
		P2IFG = ~mode_select;
	}
}
ISR_VECTOR(buttonHandler, ".int03")
