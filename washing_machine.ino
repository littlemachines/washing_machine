#include <Wire.h>
#include <U8g2lib.h>
#include <Servo.h>

// Дефиниране на пинове
#define BUTTON_START 2
#define BUTTON_UP    3
#define BUTTON_DOWN  4
#define BUTTON_LEFT  5
#define BUTTON_RIGHT 6
#define LED_PIN     13
#define SERVO_PIN    9

// Константи за OLED дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define YELLOW_ROWS 2     // Брой редове в жълтата секция
#define BLUE_ROWS 6       // Брой редове в синята секция
#define CHAR_HEIGHT 8     // Височина на един ред в пиксели
#define VISIBLE_ROWS (YELLOW_ROWS + BLUE_ROWS)  // Общ брой видими редове

// Дефинираме клас OLED за дисплея
class DualColorOLED {
private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C display; // Основен обект на U8g2
    const int yellowRows;
    const int blueRows;
    const int charHeight;

public:
    DualColorOLED(int yellowRows, int blueRows, int charHeight)
        : display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE),
          yellowRows(yellowRows), blueRows(blueRows), charHeight(charHeight) {}

    void begin() {
        display.begin();
        display.setFont(u8g2_font_6x10_tr); // опростен шрифт
        //display.setFont(u8g2_font_ncenB08_tr);
    }

    void clearDisplay() {
        display.clearBuffer();
    }

    void sendBuffer() {
        display.sendBuffer();
    }

    void setCursor(int x, int y) {
        display.setCursor(x, y);
    }

    void print(const char *text) {
        display.print(text);
    }

    void println(const char *text) {
        display.print(text);
        display.print("\n");
    }

    void printInYellowSection(const char *text, int row) {
        if (row < yellowRows) {
            setCursor(0, row * charHeight);
            print(text);
        }
    }

    void printInBlueSection(const char *text, int row) {
        if (row >= 0 && row < blueRows) {
            setCursor(0, (yellowRows + row) * charHeight);
            print(text);
        }
    }
};

// --- Инстанция на класа ---
DualColorOLED oled(YELLOW_ROWS, BLUE_ROWS, CHAR_HEIGHT); // 2 жълти реда, 4 сини реда, височина на символите 8 пиксела

// Създаване на обект за серво мотора
Servo washingDrum;

// Структура за допълнителни опции
struct WashOptions {
  bool preWash;
  bool extraWater;
  bool ecoMode;
  bool quickWash;
};

// Структура за програма за пране
struct WashProgram {
  const char* name;
  const int* temperatures;
  const int tempCount;
  const int* spins;
  const int spinCount;
  const unsigned long baseWashTime;    // Базово време за пране в милисекунди
  const unsigned long baseSpinTime;     // Базово време за центрофуга в милисекунди
  const int drumMovementIntensity;      // Интензивност на въртене (1-10)
};

// Възможни температури за различните програми
const int cottonTemp[] = {30, 40, 60, 95};
const int syntheticTemp[] = {30, 40, 60};
const int woolTemp[] = {30, 40};
const int handwashTemp[] = {30};

// Възможни обороти за различните програми
const int cottonSpin[] = {400, 800, 1000, 1200, 1400};
const int syntheticSpin[] = {400, 800, 1000};
const int woolSpin[] = {400, 800};
const int handwashSpin[] = {400};

// Дефиниране на програмите
WashProgram programs[] = {
  {"White Cotton", cottonTemp, 4, cottonSpin, 5, 45*60000, 10*60000, 8},    // 45 мин пране, 10 мин центрофуга
  {"Color Cotton", cottonTemp, 4, cottonSpin, 5, 40*60000, 10*60000, 7},    // 40 мин пране, 10 мин центрофуга
  {"Synthetic", syntheticTemp, 3, syntheticSpin, 3, 35*60000, 8*60000, 6},  // 35 мин пране, 8 мин центрофуга
  {"Wool", woolTemp, 2, woolSpin, 2, 30*60000, 5*60000, 4},                 // 30 мин пране, 5 мин центрофуга
  {"Hand Wash", handwashTemp, 1, handwashSpin, 1, 25*60000, 3*60000, 3}     // 25 мин пране, 3 мин центрофуга
};

// Структура за меню опция
struct MenuItem {
  const char* name;
  bool* value;
};

// Състояния на менюто
enum MenuState {
  PROGRAM_SELECT,
  TEMP_SELECT,
  SPIN_SELECT,
  OPTIONS_SELECT,
  WASHING
};

// Дефиниране на допълнителните опции
WashOptions washOptions = {false, false, false, false};

// Масив с опции за менюто - лесно добавяне на нови опции
MenuItem optionsMenu[] = {
  {"Pre-wash", &washOptions.preWash},
  {"Extra water", &washOptions.extraWater},
  {"Eco mode", &washOptions.ecoMode},
  {"Quick wash", &washOptions.quickWash}
};

// Глобални променливи за текущото състояние
MenuState currentState = PROGRAM_SELECT;
int selectedProgram = 0;
int selectedTemp = 0;
int selectedSpin = 0;
bool isWashing = false;
bool standbyMode = true;

// Фази на пране
const char* washPhases[] = {
  "Pre-wash",
  "Main wash",
  "Rinse",
  "Spin",
  "Final rinse"
};
int currentPhase = 0;

void setup() {
  // Стартираме серийния принт
  Serial.begin(9600); // open the serial port at 9600 bps:
  Serial.println("Program started");

  // Инициализация на пиновете
  pinMode(BUTTON_START, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("Pins initialized");
  
  // Инициализация на серво мотора
  washingDrum.attach(SERVO_PIN);
  washingDrum.write(90); // Начална позиция

  Serial.println("Servo initialized");
  
  // Инициализация на дисплея
    oled.begin();
    oled.clearDisplay();
    oled.printInYellowSection("Starting...", 0);
    oled.sendBuffer();

  Serial.println("Display initialized");
  
  // Начално показване на менюто
  updateDisplay();
}

void loop() {
  // Проверка на бутоните
  handleButtons();
  
  // Управление на LED индикатора
  updateLED();
  
  // Ако пералнята пере, въртим барабана
  if (isWashing) {
    runWashCycle();
  }

  Serial.println("End Loop");
  
  // Обновяване на дисплея
  updateDisplay();
  delay(100);
}

void handleButtons() {
  // Четене на състоянието на бутоните
  bool startPressed = !digitalRead(BUTTON_START);
  bool upPressed = !digitalRead(BUTTON_UP);
  bool downPressed = !digitalRead(BUTTON_DOWN);
  bool leftPressed = !digitalRead(BUTTON_LEFT);
  bool rightPressed = !digitalRead(BUTTON_RIGHT);

  Serial.println("Checked buttons");
  
  // Обработка според текущото състояние на менюто
  if (startPressed) {
    Serial.println("Start pressed");
    toggleWashing();
  }
  
  if (!isWashing) {
    if (upPressed) {
      navigateUp();
    }
    if (downPressed) {
      navigateDown();
    }
    if (rightPressed) {
      navigateRight();
    }
    if (leftPressed) {
      navigateLeft();
    }
  } else if (leftPressed) {
    // Спиране на програмата при задържане на левия бутон
    static unsigned long pressStart = 0;
    if (pressStart == 0) {
      pressStart = millis();
    } else if (millis() - pressStart > 2000) {
      isWashing = false;
      currentState = PROGRAM_SELECT;
      pressStart = 0;
    }
  }
}

void toggleWashing() {
  if (currentState == WASHING) {
    isWashing = !isWashing;
    if (!isWashing) {
      washingDrum.write(90);
    }
  }
}

void navigateUp() {
  Serial.println("Up Pressed");
  switch (currentState) {
    case PROGRAM_SELECT:
      if (selectedProgram > 0) selectedProgram--;
      break;
    case TEMP_SELECT:
      if (selectedTemp > 0) selectedTemp--;
      break;
    case SPIN_SELECT:
      if (selectedSpin > 0) selectedSpin--;
      break;
  }
}

void navigateDown() {
  Serial.println("Down Pressed");
  switch (currentState) {
    case PROGRAM_SELECT:
      if (selectedProgram < sizeof(programs)/sizeof(programs[0]) - 1) selectedProgram++;
      break;
    case TEMP_SELECT:
      if (selectedTemp < programs[selectedProgram].tempCount - 1) selectedTemp++;
      break;
    case SPIN_SELECT:
      if (selectedSpin < programs[selectedProgram].spinCount - 1) selectedSpin++;
      break;
  }
}

void navigateRight() {
  Serial.println("Right Pressed");
  switch (currentState) {
    case PROGRAM_SELECT:
      currentState = TEMP_SELECT;
      selectedTemp = 0;
      break;
    case TEMP_SELECT:
      currentState = SPIN_SELECT;
      selectedSpin = 0;
      break;
    case SPIN_SELECT:
      currentState = WASHING;
      isWashing = true;
      break;
  }
}

void navigateLeft() {
  Serial.println("Left Pressed");
  switch (currentState) {
    case TEMP_SELECT:
      currentState = PROGRAM_SELECT;
      break;
    case SPIN_SELECT:
      currentState = TEMP_SELECT;
      break;
    case WASHING:
      if (!isWashing) {
        currentState = SPIN_SELECT;
      }
      break;
  }
}

void updateLED() {
  if (isWashing) {
    digitalWrite(LED_PIN, HIGH);
  } else if (standbyMode) {
    // Мигане в режим на готовност
    digitalWrite(LED_PIN, (millis() / 1000) % 2);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void runWashCycle() {
  static unsigned long lastMove = 0;
  static int direction = 1;
  
  if (millis() - lastMove > (currentPhase == 3 ? 100 : 500)) {
    int speed;
    
    if (currentPhase == 3) { // Центрофуга
      speed = 180 + (70 * direction); // Бързо въртене: ~90% скорост
    } else {
      speed = 180 + (30 * direction * programs[selectedProgram].drumMovementIntensity / 10); // Нормално пране: 30-70% скорост
    }
    
    if (random(100) < 5) { // 5% шанс за смяна на посоката
      direction *= -1;
    }
    
    washingDrum.write(speed); // 0-90 обратна посока, 90-180 стоп, 180-270 права посока
    lastMove = millis();
  }
}

void updateDisplay() {
  oled.clearDisplay();
  oled.setCursor(0,0);
  
  switch (currentState) {
    case PROGRAM_SELECT:
      displayProgramSelect();
      break;
    case TEMP_SELECT:
      displayTempSelect();
      break;
    case SPIN_SELECT:
      displaySpinSelect();
      break;
    case WASHING:
      displayWashing();
      break;
  }
  
  oled.sendBuffer();
}

void displayProgramSelect() {
  // Изчисляване на индексите за скролинг
  const int totalPrograms = sizeof(programs)/sizeof(programs[0]);
  const int visibleItems = BLUE_ROWS;
  int startIdx = max(0, min(selectedProgram - visibleItems/2, totalPrograms - visibleItems));
  
  // Показване на заглавието в жълтата секция
  oled.printInYellowSection("Select Program:", 0);
  oled.printInYellowSection("<Up/Down> to scroll", 1);
  
  // Показване на програмите в синята секция
  for (int i = 0; i < visibleItems; i++) {
    int programIdx = startIdx + i;
    if (programIdx < totalPrograms) {
      char buffer[21];  // Буфер за реда (20 символа + null terminator)
      if (programIdx == selectedProgram) {
        snprintf(buffer, sizeof(buffer), "> %s", programs[programIdx].name);
      } else {
        snprintf(buffer, sizeof(buffer), "  %s", programs[programIdx].name);
      }
      oled.printInBlueSection(buffer, i);
    }
  }
  
  // Индикатори за скролинг ако има още опции
  if (startIdx > 0) {
    oled.printInYellowSection("^", 0);  // Стрелка нагоре
  }
  if (startIdx + visibleItems < totalPrograms) {
    oled.printInBlueSection("v", visibleItems-1);  // Стрелка надолу
  }
}

void displayTempSelect() {
  const int totalTemps = programs[selectedProgram].tempCount;
  const int visibleItems = BLUE_ROWS;
  int startIdx = max(0, min(selectedTemp - visibleItems/2, totalTemps - visibleItems));
  
  // Показване на заглавието в жълтата секция
  oled.printInYellowSection("Select Temperature:", 0);
  char progName[21];
  snprintf(progName, sizeof(progName), "Program: %s", programs[selectedProgram].name);
  oled.printInYellowSection(progName, 1);
  
  // Показване на температурите в синята секция
  for (int i = 0; i < visibleItems; i++) {
    int tempIdx = startIdx + i;
    if (tempIdx < totalTemps) {
      char buffer[21];
      if (tempIdx == selectedTemp) {
        snprintf(buffer, sizeof(buffer), "> %d°C", 
                programs[selectedProgram].temperatures[tempIdx]);
      } else {
        snprintf(buffer, sizeof(buffer), "  %d°C", 
                programs[selectedProgram].temperatures[tempIdx]);
      }
      oled.printInBlueSection(buffer, i);
    }
  }
  
  // Индикатори за скролинг
  if (startIdx > 0) {
    oled.printInYellowSection("^", 0);
  }
  if (startIdx + visibleItems < totalTemps) {
    oled.printInBlueSection("v", visibleItems-1);
  }
}

void displaySpinSelect() {
  const int totalSpins = programs[selectedProgram].spinCount;
  const int visibleItems = BLUE_ROWS;
  int startIdx = max(0, min(selectedSpin - visibleItems/2, totalSpins - visibleItems));
  
  // Показване на заглавието в жълтата секция
  oled.printInYellowSection("Select Spin Speed:", 0);
  char progInfo[21];
  snprintf(progInfo, sizeof(progInfo), "%s - %d°C", 
           programs[selectedProgram].name,
           programs[selectedProgram].temperatures[selectedTemp]);
  oled.printInYellowSection(progInfo, 1);
  
  // Показване на оборотите в синята секция
  for (int i = 0; i < visibleItems; i++) {
    int spinIdx = startIdx + i;
    if (spinIdx < totalSpins) {
      char buffer[21];
      if (spinIdx == selectedSpin) {
        snprintf(buffer, sizeof(buffer), "> %d rpm", 
                programs[selectedProgram].spins[spinIdx]);
      } else {
        snprintf(buffer, sizeof(buffer), "  %d rpm", 
                programs[selectedProgram].spins[spinIdx]);
      }
      oled.printInBlueSection(buffer, i);
    }
  }
  
  // Индикатори за скролинг
  if (startIdx > 0) {
    oled.printInYellowSection("^", 0);
  }
  if (startIdx + visibleItems < totalSpins) {
    oled.printInBlueSection("v", visibleItems-1);
  }
}

void displayWashing() {
  // Показване на заглавието и статуса в жълтата секция
  oled.printInYellowSection("Washing Status:", 0);
  char statusText[21];
  snprintf(statusText, sizeof(statusText), "%s", 
           isWashing ? "RUNNING" : "PAUSED");
  oled.printInYellowSection(statusText, 1);
  
  // Показване на детайлите в синята секция
  char buffer[21];
  
  // Програма и температура
  snprintf(buffer, sizeof(buffer), "%s - %d°C", 
           programs[selectedProgram].name,
           programs[selectedProgram].temperatures[selectedTemp]);
  oled.printInBlueSection(buffer, 0);
  
  // Обороти
  snprintf(buffer, sizeof(buffer), "Spin: %d rpm", 
           programs[selectedProgram].spins[selectedSpin]);
  oled.printInBlueSection(buffer, 1);
  
  // Текуща фаза
  snprintf(buffer, sizeof(buffer), "Phase: %s", 
           washPhases[currentPhase]);
  oled.printInBlueSection(buffer, 2);
  
  // Активни опции
  char optionsBuffer[21] = "Options: ";
  if (washOptions.preWash) strcat(optionsBuffer, "P");
  if (washOptions.extraWater) strcat(optionsBuffer, "W");
  if (washOptions.ecoMode) strcat(optionsBuffer, "E");
  if (washOptions.quickWash) strcat(optionsBuffer, "Q");
  if (strlen(optionsBuffer) == 9) strcat(optionsBuffer, "None");
  oled.printInBlueSection(optionsBuffer, 3);
}