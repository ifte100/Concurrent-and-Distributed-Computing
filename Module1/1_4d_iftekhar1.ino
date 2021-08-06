int state = LOW;
int MOTION = 0;
int BUTTON = 0;

int input_PIN1 = 2;
int input_PIN2 = 3;
int input_Button = 7;

int output_PIN1 = 13;
int output_PIN2 = 12;
int output_Button = 4;

void setup()
{
  pinMode(output_PIN1, OUTPUT);
  pinMode(output_PIN2, OUTPUT);

  pinMode(input_PIN1, INPUT);
  pinMode(input_PIN2, INPUT);
  pinMode(input_Button, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(input_PIN1), interrupt_func1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(input_PIN2), interrupt_func2, CHANGE);
  
  PCICR |= 0b00000100; //Turning ON port D 0-7
  PCMSK2 |= 0b10000001;    ///enable pins PCINT16-23
  
  noInterrupts();
  
  // Clear registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  
  //256 prescaler
  TCCR1B |= (1 << CS12);
  TCCR1B &= ~(1 << CS11);
  TCCR1B &= ~(1 << CS10);
  
  //Setting timer compare
  OCR1A = 31250;
  
  // Output Compare Match A Interrupt Enable
  TIMSK1 |= (1 << OCIE1A);
  
  // CTC
  TCCR1B |= (1 << WGM12);
  
  interrupts();
  Serial.begin(9600);
}

void loop()
{
  //not using loop instead interrupt
}

void interrupt_func1()
{
  MOTION = digitalRead(input_PIN1);  // read input value
  if (MOTION == HIGH)// check if the input is HIGH
  {
    digitalWrite(output_PIN1, HIGH);  // turn LED ON
    if (state == LOW) 
    {
      // we have just turned on
      Serial.println("Motion detected from first sensor!");
      // We only want to print on the output change, not state
      state = HIGH;
    }
  } 
  else 
  {
    digitalWrite(output_PIN1, LOW); // turn LED OFF
    if (state == HIGH){
      // we have just turned of
      Serial.println("Motion ended!");
      // We only want to print on the output change, not state
      state = LOW;
    }
  }
}

void interrupt_func2()
{
  MOTION = digitalRead(input_PIN2); 
  if (MOTION == HIGH)
  {
    digitalWrite(output_PIN2, HIGH);
    if (state == LOW) 
    {
      Serial.println("Motion detected from second sensor!");
      state = HIGH;
    }
  } 
  else 
  {
    digitalWrite(output_PIN2, LOW); 
    if (state == HIGH){
      Serial.println("Motion ended!");
      state = LOW;
    }
  }
}

//need PCINT2 because enables D2-D7
ISR(PCINT2_vect) {
  if(digitalRead(input_Button))
  {
    BUTTON = HIGH;
    digitalWrite(output_Button, BUTTON);
    Serial.println("BUTTON WAS PRESSED!");
  }
  else
  {
    BUTTON = LOW;
    digitalWrite(output_Button, BUTTON);
    Serial.println("button was released....");
  }
}

//every 1 second blink the in-built LED
ISR(TIMER1_COMPA_vect)
{
  digitalWrite(output_PIN1, digitalRead(output_PIN1) ^ 1);
}