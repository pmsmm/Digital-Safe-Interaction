#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Servo.h>

#define LeftArrow 8
#define RightArrow 9
#define GreenLED 3
#define RedLED 2
#define LeftKeySwitch 6
#define RightKeySwitch 7
#define Data 4
#define Clock 5
#define Push 10
#define ServoPin 11
#define SpeakerPin 12

int currentStateClock;
int lastStateClock;
unsigned long lastButtonPress = 0;

int8_t AValue, BValue, CValue = 0;
int8_t AValueSolution, BValueSolution, CValueSolution;

int8_t currentLCDIndex = 2;
bool isLCDInitiated = false;

bool INTERACTION_SOLVED, INTERACTION_RUNNING;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo;

void setup() {
  Serial.begin(9600);

  randomSeed(analogRead(A7) * analogRead(A6) + analogRead(A5));

  pinMode(Clock, INPUT_PULLUP);
  pinMode(Data, INPUT_PULLUP);
  pinMode(Push, INPUT);
  pinMode(GreenLED, OUTPUT);
  pinMode(RedLED, OUTPUT);
  pinMode(SpeakerPin, OUTPUT);
  pinMode(LeftKeySwitch, INPUT);
  pinMode(RightKeySwitch, INPUT);
  pinMode(LeftArrow, INPUT);
  pinMode(RightArrow, INPUT);

  lastStateClock = digitalRead(Clock);

  lcd.init();

  servo.attach(ServoPin);
}

void loop() {
  if (!Serial) {
    Serial.begin(9600);
  }
  if (Serial.available()) {
    processSerialMessage();
  }
  if (INTERACTION_SOLVED == false && INTERACTION_RUNNING == true) {
    gameLoop();
  }
}

void gameLoop() {
  if (digitalRead(LeftKeySwitch) == HIGH && digitalRead(RightKeySwitch) == HIGH && !isLCDInitiated) {
    initiateLCD();
  } else if (digitalRead(LeftKeySwitch) == HIGH && digitalRead(RightKeySwitch) == HIGH && isLCDInitiated) {
    encoderLoop();
    selectionMenuLoop();
  } else {
    shutdownLCD();
  }
}

void selectionMenuLoop() {
  if (digitalRead(LeftArrow) == HIGH) {
    if (currentLCDIndex == 2) {
      currentLCDIndex = 12;
    } else {
      currentLCDIndex -= 5;
    }
    lcd.setCursor(currentLCDIndex, 0);
    delay(250);
  }
  if (digitalRead(RightArrow) == HIGH) {
    if (currentLCDIndex == 12) {
      currentLCDIndex = 2;
    } else {
      currentLCDIndex += 5;
    }
    lcd.setCursor(currentLCDIndex, 0);
    delay(250);
  }
}

void encoderLoop() {
  currentStateClock = digitalRead(Clock);
  if (currentStateClock != lastStateClock  && currentStateClock == 1) {
    if (digitalRead(Data) != currentStateClock) {
      writeToLCD(1);
    } else {
      writeToLCD(-1);
    }
  }

  lastStateClock = currentStateClock;

  if (digitalRead(Push) == LOW) {
    if (millis() - lastButtonPress > 50) {
      //Button Pressed
      checkWinning();
    }
    lastButtonPress = millis();
  }
  delay(1);
}

void writeToLCD(int8_t Variation) {
  lcd.setCursor(currentLCDIndex, 0);
  lcd.print("   ");
  lcd.setCursor(currentLCDIndex, 0);
  switch (currentLCDIndex) {
    case 2:
      AValue += Variation;
      if (AValue > 100) {
        AValue = 0;
      } else if (AValue < 0) {
        AValue = 100;
      }
      lcd.print(AValue);
      break;
    case 7:
      BValue += Variation;
      if (BValue > 100) {
        BValue = 0;
      } else if (BValue < 0) {
        BValue = 100;
      }
      lcd.print(BValue);
      break;
    case 12:
      CValue += Variation;
      if (CValue > 100) {
        CValue = 0;
      } else if (CValue < 0) {
        CValue = 100;
      }
      lcd.print(CValue);
      break;
  }
}

void initiateLCD() {
  if (!isLCDInitiated) {
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.blink();
    lcd.print("A-   B-   C-   ");
    uint8_t lastCurrentLCDIndex = currentLCDIndex;
    currentLCDIndex = 2;
    writeToLCD(0);
    currentLCDIndex = 7;
    writeToLCD(0);
    currentLCDIndex = 12;
    writeToLCD(0);
    currentLCDIndex = lastCurrentLCDIndex;
    lcd.setCursor(currentLCDIndex, 0);
    isLCDInitiated = true;
  }
}

void shutdownLCD() {
  lcd.noBacklight();
  lcd.noBlink();
  lcd.clear();
  isLCDInitiated = false;
}

void processSerialMessage() {
  const uint8_t BUFF_SIZE = 64; // make it big enough to hold your longest command
  static char buffer[BUFF_SIZE + 1]; // +1 allows space for the null terminator
  static uint8_t length = 0; // number of characters currently in the buffer

  char c = Serial.read();
  if ((c == '\r') || (c == '\n')) {
    // end-of-line received
    if (length > 0) {
      tokenizeReceivedMessage(buffer);
    }
    length = 0;
  } else {
    if (length < BUFF_SIZE) {
      buffer[length++] = c; // append the received character to the array
      buffer[length] = 0; // append the null terminator
    }
  }
}

void tokenizeReceivedMessage(char *msg) {
  const uint8_t COMMAND_PAIRS = 10;
  char* tokenizedString[COMMAND_PAIRS + 1];
  uint8_t index = 0;

  char* command = strtok(msg, ";");
  while (command != 0) {
    char* separator = strchr(command, ':');
    if (separator != 0) {
      *separator = 0;
      tokenizedString[index++] = command;
      ++separator;
      tokenizedString[index++] = separator;
    }
    command = strtok(0, ";");
  }
  tokenizedString[index] = 0;

  processReceivedMessage(tokenizedString);
}

void processReceivedMessage(char** command) {
  if (strcmp(command[1], "START") == 0) {
    startSequence(command[3]);
  } else if (strcmp(command[1], "PAUSE") == 0) {
    pauseSequence(command[3]);
  } else if (strcmp(command[1], "STOP") == 0) {
    stopSequence(command[3]);
  } else if (strcmp(command[1], "INTERACTION_SOLVED_ACK") == 0) {
    setInteractionSolved();
  } else if (strcmp(command[1], "PING") == 0) {
    ping(command[3]);
  } else if (strcmp(command[1], "BAUD") == 0) {
    setBaudRate(atoi(command[3]), command[5]);
  } else if (strcmp(command[1], "SETUP") == 0) {
    Serial.println("COM:SETUP;INT_NAME:Safe Crack Interaction;BAUD:9600");
    Serial.flush();
  }
}

//TODO: Review This Method once Interaction Is Completed
void startSequence(char* TIMESTAMP) {
  INTERACTION_SOLVED = false;
  INTERACTION_RUNNING = true;
  AValueSolution = random(0, 101);
  BValueSolution = random(0, 101);
  CValueSolution = random(0, 101);
  Serial.print("COM:START_ACK;MSG:AValue-");
  Serial.print(AValueSolution);
  Serial.print(" BValue-");
  Serial.print(BValueSolution);
  Serial.print(" CValue-");
  Serial.print(CValueSolution);
  Serial.print(";ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void pauseSequence(char* TIMESTAMP) {
  INTERACTION_RUNNING = !INTERACTION_RUNNING;
  if (INTERACTION_RUNNING) {
    Serial.print("COM:PAUSE_ACK;MSG:Device is now running;ID:");
  } else {
    Serial.print("COM:PAUSE_ACK;MSG:Device is now paused;ID:");
  }
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void stopSequence(char* TIMESTAMP) {
  INTERACTION_RUNNING = false;
  Serial.print("COM:STOP_ACK;MSG:Device is now stopped;ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void setInteractionSolved() {
  INTERACTION_SOLVED = true;
  INTERACTION_RUNNING = false;
}

void ping(char* TIMESTAMP) {
  Serial.print("COM:PING;MSG:PING;ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void setBaudRate(int baudRate, char* TIMESTAMP) {
  Serial.flush();
  Serial.begin(baudRate);
  Serial.print("COM:BAUD_ACK;MSG:The Baud Rate was set to ");
  Serial.print(baudRate);
  Serial.print(";ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void checkWinning() {
  if (digitalRead(LeftKeySwitch) == HIGH && digitalRead(RightKeySwitch) == HIGH) {
    if (AValue == AValueSolution && BValue == BValueSolution && CValue == CValueSolution) {
      digitalWrite(GreenLED, HIGH);
      Serial.println("COM:INTERACTION_SOLVED;MSG:User Discovered Combination;PNT:3000");
      Serial.flush();
    } else {
      digitalWrite(RedLED, HIGH);
      delay (1000);
      digitalWrite(RedLED, LOW);
    }
  }
}
