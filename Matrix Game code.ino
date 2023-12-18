#include "LedControl.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>
int mute = 1;
const int buzzDuration = 100;
unsigned long buzzStartTime = 0;
int BUZZER_PIN = 11;
unsigned int buzzerTone = 400;
unsigned int toneDuration = 1000;
unsigned int buzzerTone_HIGH = 900;
unsigned int toneDuration_SHORT = 150;
const byte rs = 9;
const byte en = 8;
const byte d4 = 7;
const byte d5 = 3;
const byte d6 = 5;
const byte d7 = 4;
int score = 0;
int level;
int highscore_1;
int highscore_2;
int highscore_3;

char player_1[3] = "ZAC";
char player_2[3] = "BRV";
char player_3[3];
char player_name[3];
char playerName[4] = "ABC";  // The initial player name
int cursorPos = 0;           // Current position in the player name array
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int buttonPin = 2;  // Replace with your actual button pin
int buttonState = 0;

unsigned long previousMillis = 0;
const long interval = 1000;


byte settingsWheel[8] = {
  B00100,
  B01010,
  B01010,
  B00100,
  B00000,
  B00100,
  B00000,
  B00000
};

byte sign[8] = {
  B00000,
  B01110,
  B01010,
  B00010,
  B00110,
  B00100,
  B00000,
  B00100
};

int lcdBrightness;
int matrixBrightness;

int timer = 0;
int ctime;
long time = 0;
LedControl lc = LedControl(13, 12, 10, 1);  // Pins for DIN, CLK, CS, and number of MAX7219s
bool explosion = 0;
long long p_time = 0;
long long b_time = 0;
const int joyXPin = A0;  // Connect joystick X-axis to analog pin A0
const int joyYPin = A1;  // Connect joystick Y-axis to analog pin A1
const int joyButtonPin = 2;
int a, b;  //coordonates for animation
long exp_time = 0;
int var = 0;
int count = 8;
bool ammo = 1;
static int lastButtonState = HIGH;          // Initialize the last button state
static unsigned long lastDebounceTime = 0;  // Initialize the last debounce time
const int debounceDelay = 50;

long long move_time = 0;

class Player {
public:
  int x, y;
  int blinkInterval;
  bool isBlinkOn;

  Player(int startX, int startY, int blinkRate)
    : x(startX), y(startY), blinkInterval(blinkRate), isBlinkOn(true) {}

  void draw() {
    if (isBlinkOn) {
      lc.setLed(0, x, y, true);
    } else
      lc.setLed(0, x, y, false);
  }

  void erase() {
    lc.setLed(0, x, y, false);
  }

  void move(int deltaX, int deltaY) {
    erase();
    x += deltaX;
    y += deltaY;
    draw();
  }

  void updateBlink() {
    isBlinkOn = !isBlinkOn;
  }
};

class Bomb {
public:
  int x, y;
  int blinkInterval;
  bool isBlinkOn;

  Bomb(int startX, int startY, int blinkRate)
    : x(startX), y(startY), blinkInterval(blinkRate), isBlinkOn(true) {}

  void draw() {
    if (isBlinkOn) {
      lc.setLed(0, x, y, true);
    } else
      lc.setLed(0, x, y, false);
  }

  void erase() {
    lc.setLed(0, x, y, false);
  }

  void updateBlink() {
    isBlinkOn = !isBlinkOn;
  }
};

class Wall {
public:
  int x, y;

  // Default constructor
  Wall()
    : x(0), y(0) {}

  // Parameterized constructor
  Wall(int startX, int startY)
    : x(startX), y(startY) {}

  void draw() {
    lc.setLed(0, x, y, true);
  }
  void erase() {
    lc.setLed(0, x, y, false);
  }
};


const int maxWalls = 8;
Wall walls[maxWalls] = {
  Wall(1, 2),
  Wall(4, 5),
  Wall(7, 3),
  Wall(4, 2),
  Wall(6, 5),
  Wall(8, 3),
  Wall(3, 2),
  Wall(2, 5),


};
int buzzFrequency = 400;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  lc.shutdown(0, false);
  lcd.createChar(5, settingsWheel);
  lcd.createChar(6, sign);
  lcd.begin(16, 2);
  pinMode(6, OUTPUT);
  mute = EEPROM.read(15);
  lcdBrightness = EEPROM.read(0);
  matrixBrightness = EEPROM.read(1);
  highscore_1 = EEPROM.read(2);
  highscore_2 = EEPROM.read(3);
  highscore_3 = EEPROM.read(4);
  player_1[0] = EEPROM.read(5);
  player_1[1] = EEPROM.read(6);
  player_1[2] = EEPROM.read(7);
  player_2[0] = EEPROM.read(8);
  player_2[1] = EEPROM.read(9);
  player_2[2] = EEPROM.read(10);



  lc.setIntensity(0, matrixBrightness);
  analogWrite(6, lcdBrightness);
  unsigned long currentMillis = millis();
  while (currentMillis - previousMillis <= interval) {
    showIntroMessage();
    currentMillis = millis();
  }


  pinMode(buttonPin, INPUT);
  Serial.begin(9600);  // Initialize Serial communication

  // Load brightness values from EEPROM on system start
  Serial.begin(9600);
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  pinMode(joyXPin, INPUT);
  pinMode(joyYPin, INPUT);
  pinMode(joyButtonPin, INPUT_PULLUP);
}

void drawAllEntities(Player& player, Bomb& bomb) {
  player.draw();
  bomb.draw();
  for (int i = 0; i < maxWalls; ++i) {
    walls[i].draw();
  }
}



Player player(6, 6, 300);  // Initial position for the player with a blink interval of 1000ms
Bomb bomb(-1, -1, 100);    // Initial position for the bomb with a blink interval of 500ms



void isCollision(Player& player, Bomb& bomb) {



  for (int i = 0; i < maxWalls; i++) {
    if (((abs(bomb.x - walls[i].x) == 1 && bomb.y == walls[i].y) || (bomb.x == walls[i].x && abs(bomb.y - walls[i].y) == 1)) && explosion == 1) {
      walls[i].x = -1, walls[i].y = -1, walls[i].draw(), count = count - 1, score = score + level, analogWrite(6, lcdBrightness);
      if(mute==1)
        tone(BUZZER_PIN, buzzerTone_HIGH, toneDuration_SHORT);
    }
  }
  if (((abs(bomb.x - player.x) == 1 && bomb.y == player.y) || (bomb.x == player.x && abs(bomb.y - player.y) == 1)) && explosion == 1) {
    if(mute==1)
        tone(BUZZER_PIN, buzzerTone, toneDuration); 
    analogWrite(6, lcdBrightness), analogWrite(6, lcdBrightness), endGame();
    
  }
}


void handleJoystick(Player& player) {
  const int joyX = analogRead(joyXPin);
  const int joyY = analogRead(joyYPin);

  
  const int deadZone = 100;

  // Map joystick values to player movement, considering the dead zone
  int deltaX = 0;
  int deltaY = 0;

  if (abs(joyX - 512) > deadZone) {
    deltaX = (joyX < 512) ? -1 : 1;
  }

  if (abs(joyY - 512) > deadZone) {
    deltaY = (joyY < 512) ? 1 : -1;
  }

  
  const int movementSpeed = 200;
  long currentTime = millis();

  if (currentTime - move_time > movementSpeed) {
    if (!isWallInDirection(player, deltaX, deltaY)) {

      player.move(deltaX, deltaY);
    }
    move_time = currentTime;
  }
}

void loop() {

  showMainMenu();
}




void reset() {
  eraseAllEntities(player, bomb);
  walls[1].x = 1;
  walls[1].y = 2;
  walls[2].x = 4;
  walls[2].y = 5;
  walls[3].x = 7;
  walls[3].y = 3;
  walls[4].x = 4;
  walls[4].y = 2;
  walls[5].x = 6;
  walls[5].y = 5;
  walls[6].x = 8;
  walls[6].y = 3;
  walls[7].x = 3;
  walls[7].y = 2;
  walls[8].x = 2;
  walls[8].y = 5;


  player.x = 6;
  player.y = 6;
  explosion = 0;
  count = 8;
}


bool isWallInDirection(Player& player, int deltaX, int deltaY) {
  
  int newX = player.x + deltaX;
  int newY = player.y + deltaY;

  
  if (newX < 0 || newX >= 8 || newY < 0 || newY >= 8) {
    return true;  // The move is out of bounds, treat it as a wall
  }

  // Check if there is a wall at the new position
  for (const auto& wall : walls) {
    if (wall.x == newX && wall.y == newY) {
      return true;  // There is a wall in the direction of the move
    }
  }

  return false;  // No wall in the direction of the move
}


void eraseAllEntities(Player& player, Bomb& bomb) {
  player.erase();
  bomb.erase();
  for (int i = 0; i < maxWalls; ++i) {
    walls[i].erase();
  }
}



void showIntroMessage() {
  lcd.clear();
  unsigned long currentMillis = millis();

  lcd.setCursor(0, 0);
  lcd.print("Welcome to the");
  lcd.setCursor(0, 1);
  lcd.print("BOMB MAZE!");
}

int readChoice() {
  while (!Serial.available()) {
    // Wait for user input in Serial Monitor
  }
  return Serial.parseInt();  // Read the user's choice as an integer
}

void showMainMenu() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1.Start 2.Sett.");
  lcd.setCursor(15, 0);
  lcd.write((byte)5);  // Display the settings wheel

  lcd.setCursor(0, 1);
  lcd.print("3.About 4.GameST.");
  lcd.setCursor(6, 1);
  lcd.write((byte)6);
  // Read user choice from Serial Monitor
  int choice = readChoice();

  // Handle user selection
  switch (choice) {
    case 1:
      instructions();
      break;
    case 2:
      showSettingsMenu();
      break;
    case 3:
      showAbout();
      break;
    case 4:
      GameSettings();
      break;
  }
}

void startGame() {
  level = 1;
  score = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1 live");
  lcd.setCursor(7, 0);
  lcd.print("Level=");
  lcd.setCursor(14, 0);
  lcd.print(level);
  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(score);
  handleJoystickInput();
  while (true) {
    if (count == 1)
      level_2();
    drawAllEntities(player, bomb);
    handleJoystick(player);
    isCollision(player, bomb);
    analogWrite(6, lcdBrightness);
    while (var == -1) {

      lc.setLed(0, 2, 3, true);
      lc.setLed(0, 5, 3, true);

      // Mouth
      lc.setLed(0, 2, 6, true);
      lc.setLed(0, 3, 5, true);
      lc.setLed(0, 4, 5, true);
      lc.setLed(0, 5, 6, true);

      if (millis() - ctime > 2500) {
        var = 0;
        lc.setLed(0, 2, 3, false);
        lc.setLed(0, 5, 3, false);

        // Mouth
        lc.setLed(0, 2, 6, false);
        lc.setLed(0, 3, 5, false);
        lc.setLed(0, 4, 5, false);
        lc.setLed(0, 5, 6, false);
        reset();
      }
    }

    while (var == 1) {
      lc.setLed(0, 2, 3, true);
      lc.setLed(0, 5, 3, true);

      // Mouth
      lc.setLed(0, 2, 5, true);
      lc.setLed(0, 3, 6, true);
      lc.setLed(0, 4, 6, true);
      lc.setLed(0, 5, 5, true);
      if (millis() - ctime > 2500) {
        var = 0;
        lc.setLed(0, 2, 3, false);
        lc.setLed(0, 5, 3, false);

        // Mouth
        lc.setLed(0, 2, 5, false);
        lc.setLed(0, 3, 6, false);
        lc.setLed(0, 4, 6, false);
        lc.setLed(0, 5, 5, false);
        reset();
      }
    }



    long long player_time = millis();

    if (player_time - p_time > player.blinkInterval) {
      player.updateBlink();
      p_time = player_time;
      drawAllEntities(player, bomb);
    }

    long long bomb_time = millis();
    if (bomb_time - b_time > bomb.blinkInterval) {
      bomb.updateBlink();
      b_time = bomb_time;
      drawAllEntities(player, bomb);
    }

    int buttonState = digitalRead(joyButtonPin);



    if (buttonState != lastButtonState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      // If the button state has changed and is stable for the debounce delay
      if (buttonState == LOW) {
        // Spawn a bomb at the player's position
        if (ammo == 1) {
          bomb.x = player.x;
          bomb.y = player.y;
        }
        timer = 1;
        time = millis();
        ammo = 0;
      }

      if (timer == 0)
        exp_time = 0;
      else
        exp_time = millis();
      if (exp_time - time > 3000 && exp_time - time < 3350) {
        lc.setLed(0, bomb.x, bomb.y + 1, true);
        lc.setLed(0, bomb.x - 1, bomb.y, true);
        lc.setLed(0, bomb.x + 1, bomb.y, true);
        lc.setLed(0, bomb.x, bomb.y - 1, true);
        lc.setLed(0, bomb.x, bomb.y, true);
        explosion = true;
      }
      if (exp_time - time > 3350 && exp_time - time < 3500) {
        lc.setLed(0, bomb.x, bomb.y + 1, false);
        lc.setLed(0, bomb.x - 1, bomb.y, false);
        lc.setLed(0, bomb.x + 1, bomb.y, false);
        lc.setLed(0, bomb.x, bomb.y - 1, false);
        if (bomb.x != -1) {
          a = bomb.x;  //animation
          b = bomb.y;  //animation
        }
        bomb.x = -1;
        bomb.y = -1;
      }
      if (exp_time - time > 3700) {
        lc.setLed(0, a, b, false);
        explosion = false;
        timer = 0;
        ammo = 1;  //you can have just one bomb at a time
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("1 live");
      lcd.setCursor(8, 0);
      lcd.print("Level=");
      lcd.setCursor(14, 0);
      lcd.print(level);
      lcd.setCursor(0, 1);
      lcd.print("Score: ");
      lcd.print(score);
    }

    lastButtonState = buttonState;
  }
  showMainMenu();
}

void showSettingsMenu() {
  Serial.read();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1. LCD Brightness");
  lcd.setCursor(0, 1);
  lcd.print("2. Matrix Brightness");

  // Read user choice from Serial Monitor
  int choice;

  while (choice != 3) {
    choice = readChoice();
    switch (choice) {
      case 1:
        lcdBrightness = adjustBrightness("LCD"), analogWrite(6, lcdBrightness), EEPROM.write(0, lcdBrightness), showSettingsMenu();

        break;
      case 2:
        matrixBrightness = adjustBrightness("Matrix") / 10, lc.setIntensity(0, matrixBrightness), EEPROM.write(1, matrixBrightness), showSettingsMenu();
        break;
      case 3:
        break;
    }
  }
  showMainMenu();
}

int adjustBrightness(const char* device) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Adjusting ");
  lcd.print(device);
  lcd.print(" Brightness");

  lcd.setCursor(0, 1);
  lcd.print("Use arrow keys");

  int brightness = 0;
  int arrowKey = 0;

  while (arrowKey != '1') {  // '\r' is the Enter key
    if (Serial.available()) {
      arrowKey = Serial.read();

      switch (arrowKey) {
        case 'A':                                  // Up arrow key
          brightness = min(brightness + 10, 255);  // Increase brightness (up to 255)
          break;
        case 'B':                                // Down arrow key
          brightness = max(brightness - 10, 0);  // Decrease brightness (down to 0)
          break;
      }

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Adjusting ");
      lcd.print(device);
      lcd.print(" Brightness");

      lcd.setCursor(0, 1);
      lcd.print("Brightness: ");
      lcd.print(brightness);
    }
  }

  return brightness;
}

void showAbout() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alex Nicorescu");
  lcd.setCursor(0, 1);
  lcd.print("https://t.ly/hfXls");
  // Display information about the game and its creators
  while (Serial.parseInt() != 1) {
    // Wait for the button to be pressed
  }

  showMainMenu();
}

void endGame() {
  readChoice();
  if (score > highscore_1)
    highscore_1 = score, strcpy(player_1, player_name);
  else if (score > highscore_2)
    highscore_2 = score, strcpy(player_2, player_name);
  else if (score > highscore_3)
    highscore_3 = score, strcpy(player_3, player_name);
  EEPROM.write(2, highscore_1);
  EEPROM.write(3, highscore_2);
  EEPROM.write(4, highscore_3);
  EEPROM.write(5, player_1[0]);
  EEPROM.write(6, player_1[1]);
  EEPROM.write(7, player_1[2]);
  EEPROM.write(8, player_2[0]);
  EEPROM.write(9, player_2[1]);
  EEPROM.write(10, player_2[2]);
  score = 0;
  high_score();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Game Over");
  lcd.setCursor(0, 1);
  lcd.print("Press 1 to rsrt");

  // Wait for user prompt (e.g., button press) before returning to main menu
  while (Serial.parseInt() != 1) {
    // Wait for the button to be pressed
  }

  eraseAllEntities(player, bomb);

  ctime = millis();
  lc.setLed(0, bomb.x, bomb.y + 1, false);
  lc.setLed(0, bomb.x - 1, bomb.y, false);
  lc.setLed(0, bomb.x + 1, bomb.y, false);
  lc.setLed(0, bomb.x, bomb.y - 1, false);
  lc.setLed(0, bomb.x, bomb.y, false);
  bomb.x = -1, bomb.y = -1;
  int empty = readChoice();
  showMainMenu();
}

void handleJoystickInput() {
  lcd.clear();

  while (Serial.parseInt() != 1) {
    lcd.clear();
    int joystickX = analogRead(joyXPin);
    int joystickY = analogRead(joyYPin);
    lcd.setCursor(cursorPos + 6, 0);
    lcd.write('_');
    // Adjust the cursor position based on joystick movement
    if (joystickY < 100) {
      cursorPos = (cursorPos + 1) % 3;
    } else if (joystickY > 900) {
      cursorPos = (cursorPos + 2) % 3;
    }

    if (joystickX < 100) {
      playerName[cursorPos] = incrementLetter(playerName[cursorPos]);
    } else if (joystickX > 900) {
      playerName[cursorPos] = decrementLetter(playerName[cursorPos]);
    }

    // Display the updated player name
    lcd.setCursor(0, 0);
    lcd.print("Press");
    lcd.setCursor(10, 0);
    lcd.print("1 ");
    lcd.setCursor(0, 1);
    lcd.print("Name: ");
    lcd.setCursor(6, 1);
    lcd.print(playerName);
    strcpy(player_name, playerName);
  }
}
// Helper function to increment a letter (handles wrapping from Z to A)
char incrementLetter(char letter) {
  if (letter == 'Z') {
    return 'A';
  } else {
    return letter + 1;
  }
}

// Helper function to decrement a letter (handles wrapping from A to Z)
char decrementLetter(char letter) {
  if (letter == 'A') {
    return 'Z';
  } else {
    return letter - 1;
  }
}
void high_score() {
  lcd.clear();
  while (Serial.parseInt() != 1) {
    lcd.setCursor(0, 0);
    lcd.print("1. N=");
    lcd.print(player_1);
    lcd.setCursor(8, 0);
    lcd.print(" S=");
    lcd.setCursor(11, 0);
    lcd.print(highscore_1);
    lcd.setCursor(0, 1);
    lcd.print("2. N=");
    lcd.print(player_2);
    lcd.setCursor(8, 1);
    lcd.print(" S=");
    lcd.setCursor(11, 1);
    lcd.print(highscore_2);
  }
  showMainMenu();
}
void level_2() {
  {
    lc.setLed(0, bomb.x, bomb.y + 1, false);
    lc.setLed(0, bomb.x - 1, bomb.y, false);
    lc.setLed(0, bomb.x + 1, bomb.y, false);
    lc.setLed(0, bomb.x, bomb.y - 1, false);
    if (bomb.x != -1) {
      a = bomb.x;  //animation
      b = bomb.y;  //animation
    }
    bomb.x = -1;
    bomb.y = -1;
    lc.setLed(0, a, b, false);
  }

  reset();
  count = 8;
  level = 2;
  lcd.clear();
  int current_time = millis();
  while (millis() - current_time < 3000) {
    lcd.setCursor(0, 1);
    lcd.print("LEVEL 2");
    lcd.setCursor(0, 0);
    lcd.print("PRESS 1");
  }
  int nr=0, a, b, c;
  a = 0, b = 0, c = 0;
  while (Serial.parseInt() != 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You did better than");
    lcd.setCursor(0, 1);

    if (score > highscore_1 && a == 0 && highscore_1>0)
      nr++, a = 1;
    if (score > highscore_2 && b == 0  && highscore_2>0)
      nr++, b = 1;
    if (score > highscore_3 && c == 0  && highscore_3>0)
      nr++, c = 1;
    lcd.print(nr);
    lcd.setCursor(4, 1);
    lcd.print("players!");
  }

  while (true) {
    if (score == 19)
       win();
    drawAllEntities(player, bomb);
    handleJoystick(player);
    isCollision(player, bomb);




    long long player_time = millis();

    if (player_time - p_time > player.blinkInterval) {
      player.updateBlink();
      p_time = player_time;
      drawAllEntities(player, bomb);
    }

    long long bomb_time = millis();
    if (bomb_time - b_time > bomb.blinkInterval) {
      bomb.updateBlink();
      b_time = bomb_time;
      drawAllEntities(player, bomb);
    }

    int buttonState = digitalRead(joyButtonPin);



    if (buttonState != lastButtonState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      // If the button state has changed and is stable for the debounce delay
      if (buttonState == LOW) {
        // Spawn a bomb at the player's position
        if (ammo == 1) {
          bomb.x = player.x;
          bomb.y = player.y;
        }
        timer = 1;
        time = millis();
        ammo = 0;
      }

      if (timer == 0)
        exp_time = 0;
      else
        exp_time = millis();
      if (exp_time - time > 600 && exp_time - time < 750) {
        lc.setLed(0, bomb.x, bomb.y + 1, true);
        lc.setLed(0, bomb.x - 1, bomb.y, true);
        lc.setLed(0, bomb.x + 1, bomb.y, true);
        lc.setLed(0, bomb.x, bomb.y - 1, true);
        lc.setLed(0, bomb.x, bomb.y, true);
        explosion = true;
      }
      if (exp_time - time > 750 && exp_time - time < 950) {
        lc.setLed(0, bomb.x, bomb.y + 1, false);
        lc.setLed(0, bomb.x - 1, bomb.y, false);
        lc.setLed(0, bomb.x + 1, bomb.y, false);
        lc.setLed(0, bomb.x, bomb.y - 1, false);
        if (bomb.x != -1) {
          a = bomb.x;  //animation
          b = bomb.y;  //animation
        }
        bomb.x = -1;
        bomb.y = -1;
      }
      if (exp_time - time > 1400) {
        lc.setLed(0, a, b, false);
        explosion = false;
        timer = 0;
        ammo = 1;  //you can have just one bomb at a time
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("1 live");
      lcd.setCursor(8, 0);
      lcd.print("Level=");
      lcd.setCursor(14, 0);
      lcd.print(level);
      lcd.setCursor(0, 1);
      lcd.print("Score: ");
      lcd.print(score);
    }

    lastButtonState = buttonState;
  }
  showMainMenu();
}

void buzzWithoutDelay() {
  // Set the frequency for the buzz sound
  int buzzFrequency = 1000;  // Adjust this value for your desired frequency

  // Check if it's time to stop the buzz
  if (millis() - buzzStartTime < buzzDuration) {
    // Use analogWrite to modulate the buzzer frequency
    analogWrite(BUZZER_PIN, buzzFrequency);
  } else {
    // Stop the buzzer
    noTone(BUZZER_PIN);

    // Pause for a moment before the next sound
    delay(500);

    // Reset the start time for the next buzz
    buzzStartTime = millis();
  }
}

void GameSettings() {

  lcd.clear();
  lcd.setCursor(0, 0);
  if (mute == 1)
    lcd.print("Press 1 to mute");
  else
    lcd.print("Press 1 to unmute");

  lcd.setCursor(0, 1);
  lcd.print("Press 2 to reset highscore");
  int aux=Serial.parseInt();
  while (aux!=3) {
    int choice = readChoice();
    aux=choice;
    // Handle user selection
    switch (choice) {
      case 1:
        mute = -mute;
        EEPROM.write(15, mute);
        break;
      case 2:
        reset_highscore();
        break;    
        
    }
  }
  showMainMenu();
}

void reset_highscore() {
  EEPROM.write(2, 0);
  EEPROM.write(3, 0);
  EEPROM.write(4, 0);
  EEPROM.write(5, 'N');
  EEPROM.write(6, 'N');
  EEPROM.write(7, 'N');
  EEPROM.write(8, 'N');
  EEPROM.write(9, 'N');
  EEPROM.write(10, 'N');
}
void instructions(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("How to play");
  lcd.setCursor(0, 1);
  lcd.print("Press and run ;)");
  // Display information about the game and its creators
  while (Serial.parseInt() != 1) {
    // Wait for the button to be pressed
  }

  startGame();
}

void win(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("You won!");
  lcd.setCursor(0, 1 );
  lcd.print("Good job!");
  // Display information about the game and its creators
  eraseAllEntities(player, bomb);
  ctime = millis();
  lc.setLed(0, bomb.x, bomb.y + 1, false);
  lc.setLed(0, bomb.x - 1, bomb.y, false);
  lc.setLed(0, bomb.x + 1, bomb.y, false);
  lc.setLed(0, bomb.x, bomb.y - 1, false);
  lc.setLed(0, bomb.x, bomb.y, false);
  bomb.x = -1, bomb.y = -1;
  var = 1;
  while (Serial.parseInt() != 1) {
    // Wait for the button to be pressed
  }
  reset();
showMainMenu();

}
