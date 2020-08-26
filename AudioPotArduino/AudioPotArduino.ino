#define SENSITIVITY 2
#define DELAY 70
#define BAUD 9600
#define PIN A6

void setup() {
  Serial.begin(BAUD);
  Serial.println("m");
}

void loop() {
  int prev_val = -1, val = 0;
  while (1)
  {
    val = analogRead(PIN);
    if (abs(prev_val - val) > SENSITIVITY && prev_val != -1)
    {
      Serial.println(1023 - val);
    }
    prev_val = val;
    delay(DELAY);
  }
}
