// C++ code
//
int state = LOW;
int MOTION = 0;
void setup()
{
  pinMode(13, OUTPUT);
  pinMode(2, INPUT);
}

void loop()
{
  MOTION = digitalRead(2);  // read input value
  if (MOTION == HIGH)// check if the input is HIGH
  {
    digitalWrite(13, HIGH);  // turn LED ON
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
    digitalWrite(13, LOW); // turn LED OFF
    if (state == HIGH){
      // we have just turned of
      Serial.println("Motion ended!");
      // We only want to print on the output change, not state
      state = LOW;
    }
  }
  
}