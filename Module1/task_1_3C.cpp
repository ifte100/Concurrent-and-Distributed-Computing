int state = LOW;
int MOTION = 0;

int input_PIN1 = 2;
int input_PIN2 = 3;

int output_PIN1 = 13;
int output_PIN2 = 12;

void setup()
{
  Serial.begin(9600);
  pinMode(output_PIN1, OUTPUT);
  pinMode(output_PIN2, OUTPUT);

  pinMode(input_PIN1, INPUT);
  pinMode(input_PIN2, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(input_PIN1), interrupt_func1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(input_PIN2), interrupt_func2, CHANGE);
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