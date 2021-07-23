int state = LOW;
int MOTION = 0;
int input_PIN = 2;
int output_PIN = 13;

void setup()
{
  Serial.begin(9600);
  pinMode(output_PIN , OUTPUT);
  pinMode(input_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(input_PIN), interrupt_func, CHANGE);
}

void loop()
{
  //not using loop instead interrupt
}

void interrupt_func()
{
  MOTION = digitalRead(input_PIN);  // read input value
  if (MOTION == HIGH)// check if the input is HIGH
  {
    digitalWrite(output_PIN, HIGH);  // turn LED ON
    if (state == LOW) 
    {
      // we have just turned on
      Serial.println("Motion detected!");
      // We only want to print on the output change, not state
      state = HIGH;
    }
  } 
  else 
  {
    digitalWrite(output_PIN, LOW); // turn LED OFF
    if (state == HIGH){
      // we have just turned of
      Serial.println("Motion ended!");
      // We only want to print on the output change, not state
      state = LOW;
    }
  }
}