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
const int cottonECOTemp[] = {40, 60};
const int cottonTemp[] = {0, 20, 30, 40, 50, 60, 75, 90};
const int ECOTemp[] = {};
const int MinimumironTemp[] = {0, 20, 30, 40, 50, 60};
const int ShirtsTemp[] = {0, 20, 30, 40};
const int JeansTemp[] = {0, 20, 30, 40};
const int DelicatesTemp[] = {0, 20, 30, 40, 50, 60};
const int WoollensTemp[] = {0, 20, 30, 40};
const int SilksTemp[] = {0, 20, 30};
const int PillowsTemp[] = {0, 20, 30, 40, 50, 60};
const int DownfilleditemsTemp[] = {0, 20, 30, 40, 50, 60};
const int TrainersTemp[] = {0, 20, 30, 40};
// Възможни обороти за различните програми
const int cottonECOSpin[] = {0, 400, 600, 800, 1000, 1200, 1400, 1600};
const int cottonSpin[] = {0, 400, 600, 800, 1000, 1200, 1400, 1600};
const int ECOSpin[] = {0, 400, 600, 800, 1000, 1200, 1400, 1600};
const int MinimumironSpin[] = {0, 400, 600, 800, 1000, 1200};
const int ShirtsSpin[] = {400, 600, 900};
const int JeansSpin[] = {0, 400, 600, 900};
const int DelicatesSpin[] = {0, 400, 600, 900};
const int WoollensSpin[] = {0, 400, 600, 800, 1000, 1200};
const int SilksSpin[] = {0, 400, 600};
const int PillowsSpin[] {0, 400, 600, 800, 1000, 1200};
const int DownfilleditemsSpin[] = {0, 400, 600, 800, 1000, 1200};
const int TrainersSpin[] = {0, 600};
// Дефиниране на програмите
WashProgram programs[] = {
{"Cotton ECO", cottonECOTemp, sizeof(cottonECOTemp)/sizeof(cottonECOTemp[0]), 
                 cottonECOSpin, sizeof(cottonECOSpin)/sizeof(cottonECOSpin[0]), 
                 106*60000, 12*60000},

{"Cotton", cottonTemp, sizeof(cottonTemp)/sizeof(cottonTemp[0]), 
                 cottonSpin, sizeof(cottonSpin)/sizeof(cottonSpin[0]), 
                 74*60000, 11*60000},
{"ECO", ECOTemp, 0, ECOSpin, 8, 104*60000, 11*60000},
{"Minimum iron", MinimumironTemp, 6, MinimumironSpin, 6, 76*60000, 4*60000},
{"Shirts", ShirtsTemp, 4, ShirtsSpin, 3, 76*60000, 4*60000},
{"Jeans", JeansTemp, 4, JeansSpin, 4, 76*60000, 6*60000},
{"Delicates", DelicatesTemp, 6, DelicatesSpin, 4, 54*60000, 5*60000},
{"Woollens", WoollensTemp, 4, WoollensSpin, 6, 36*60000, 3*60000},
{"Silks", SilksTemp, 3, SilksSpin, 3, 33*60000, 3*60000},
{"Pillows", PillowsTemp, 6, PillowsSpin, 6, 76*60000, 6*60000},
{"Down filled items", DownfilleditemsTemp, 6, DownfilleditemsSpin, 6, 54*60000, 6*60000},
{"Trainers", TrainersTemp, 4, TrainersSpin, 2, 54*60000, 3*60000}
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

// Гобални променливи за текущото състояние
MenuState currentState = PROGRAM_SELECT;
int selectedProgram = 0;
int selectedTemp = 0;
int selectedSpin = 0;
bool isWashing = false;
bool standbyMode = true;

// Изброяване на всички възможни фази
enum WashPhase {
  SOAK,              // Накисване
  PRE_WASH,          // Предпране
  PRE_WASH_SPIN,     // Предпране центрофуга
  MAIN_WASH,         // Основно пране
  COOLING,           // Охлаждане
  MAIN_WASH_SPIN,    // Основно пране центрофуга
  RINSE_1,           // Първо изплакване
  RINSE_1_SPIN,      // Първо изплакване центрофуга
  RINSE_2,           // Второ изплакване
  RINSE_2_SPIN,      // Второ изплакване центрофуга
  RINSE_3,           // Трето изплакване
  DRAIN,             // Източване
  FINAL_SPIN,        // Финална центрофуга
  ANTI_WRINKLE,      // Против намачкване
  STEAM              // Пара
};

// Структура за конфигурация на фаза
struct PhaseConfig {
  const char* name;
  float timePercent;
  bool isSpinPhase;
};

// Конфигурации за всички фзи
const PhaseConfig phaseConfigs[] = {
  {"Soak",         0.10, false},  // SOAK
  {"Pre-wash",     0.15, false},  // PRE_WASH
  {"Pre-wash, spin",     0.30, true},   // PRE_WASH_SPIN
  {"Main wash",    0.30, false},  // MAIN_WASH
  {"Cooling",      0.05, false},  // COOLING
  {"Main wash, spin",    0.50, true},   // MAIN_WASH_SPIN
  {"Rinse 1",      0.10, false},  // RINSE_1
  {"Rinse 1 spin", 0.40, true},   // RINSE_1_SPIN
  {"Rinse 2",      0.10, false},  // RINSE_2
  {"Rinse 2 spin", 0.40, true},   // RINSE_2_SPIN
  {"Rinse 3",      0.10, false},  // RINSE_3
  {"Drain",        0.01, false},  // DRAIN
  {"Final spin",   1.00, true},   // FINAL_SPIN
  {"Anti-wrinkle", 0.10, false},  // ANTI_WRINKLE
  {"Steam", 0.30, false} // STEAM
};

// Активни фази в текущия цикъл на пране (тук можете лесно да добавяте/премахвате фази)
const WashPhase washPhases[] = {
  // SOAK,
  // PRE_WASH,
  // PRE_WASH_SPIN,
  MAIN_WASH,
 // COOLING,
  MAIN_WASH_SPIN,
  RINSE_1,
  RINSE_1_SPIN,
  RINSE_2,
 // RINSE_2_SPIN,
 // RINSE_3,
  DRAIN,
  FINAL_SPIN,
  ANTI_WRINKLE,
//  STEAM
};


int currentPhase = 0;

// В началото на файла с другите глобални променливи
unsigned long washStartTime = 0;

// Константи за серво мотора
const int SERVO_STOP = 1500;      // Централна позиция (стоп)
const int INITIAL_RAMP = 5000;    // 5 секунди за ускорение
const int SLOW_RAMP = 120000;      // 120 секунди за бавно ускорение
const int WASH_SPEED = 100;       // Скорост за нормално пране

// Константи за междинните центрофуги
const int VERY_LOW_SPIN = 100;    // Много ниски обороти в началото
const int INITIAL_SPIN = 500;     // Първо ниво
const int MEDIUM_SPIN = 700;      // Второ ниво
const int MAX_INTERIM_SPIN = 1000; // Максимум за междинни центрофуги

// Константи за финалната центрофуга
const int FINAL_INITIAL_SPIN = 900;  // Начално ниво за финална центрофуга
const int FINAL_MEDIUM_SPIN = 1200;  // Средно ниво за финална центрофуга

// Времена за задържане
const unsigned long VERY_LOW_HOLD = 7000;  // 5 секунди на много ниски обороти
const unsigned long HOLD_TIME = 30000;     // 30 секунди задържане
const unsigned long FINAL_HOLD_900_TIME = 120000;  // 2 минути за финална
const unsigned long FINAL_HOLD_1200_TIME = 180000; // 3 минути за финална

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

void runWashCycle() {
    if (!isWashing) return;

    const PhaseConfig& currentPhaseConfig = phaseConfigs[washPhases[currentPhase]];
    
    if (shouldSkipSpinPhase(currentPhaseConfig)) {
        moveToNextPhase();
        return;
    }
    
    unsigned long phaseTime = calculatePhaseTime(currentPhaseConfig);
    updateDrumMovement(currentPhaseConfig);
    
    if (isPhaseComplete()) {
        handlePhaseCompletion();
    }
}

// Помощни функции за runWashCycle
bool shouldSkipSpinPhase(const PhaseConfig& phaseConfig) {
    return phaseConfig.isSpinPhase && programs[selectedProgram].spins[selectedSpin] == 0;
}

unsigned long calculatePhaseTime(const PhaseConfig& phaseConfig) {
    if (phaseConfig.isSpinPhase) {
        return programs[selectedProgram].baseSpinTime * phaseConfig.timePercent;
    }
    return programs[selectedProgram].baseWashTime * phaseConfig.timePercent;
}

void updateDrumMovement(const PhaseConfig& phaseConfig) {
    static unsigned long lastSpinTime = 0;
    unsigned long currentTime = millis();
    
    if (!phaseConfig.isSpinPhase) {
        // Нормално пране - редуване на посоката на всеки 5 секунди
        if ((currentTime / 5000) % 2 == 0) {
            washingDrum.writeMicroseconds(SERVO_STOP + WASH_SPEED);
        } else {
            washingDrum.writeMicroseconds(SERVO_STOP - WASH_SPEED);
        }
        lastSpinTime = currentTime;
        return;
    }

    // Центрофуга
    if (lastSpinTime == 0) {
        lastSpinTime = currentTime;
    }

    unsigned long elapsedTime = currentTime - lastSpinTime;
    int targetSpeed = programs[selectedProgram].spins[selectedSpin];
    int currentSpeed;

    // Проверяваме дали това е финалната центрофуга или Rinse 2 spin
    bool isFinalSpin = (currentPhase == sizeof(washPhases)/sizeof(washPhases[0]) - 2);
    bool isRinse2Spin = (strcmp(phaseConfig.name, "Rinse 2 spin") == 0);

    if (isFinalSpin) {
        // Логика за финалната центрофуга
        if (elapsedTime < VERY_LOW_HOLD) {
            // Започваме с много ниски обороти
            currentSpeed = VERY_LOW_SPIN;
        }
        else if (elapsedTime < VERY_LOW_HOLD + INITIAL_RAMP) {
            // Плавно до 900
            float progress = (float)(elapsedTime - VERY_LOW_HOLD) / INITIAL_RAMP;
            currentSpeed = (int)(FINAL_INITIAL_SPIN * progress);
        }
        else if (elapsedTime < VERY_LOW_HOLD + INITIAL_RAMP + FINAL_HOLD_900_TIME) {
            // Задържане на 900
            currentSpeed = FINAL_INITIAL_SPIN;
        }
        else if (elapsedTime < VERY_LOW_HOLD + INITIAL_RAMP + FINAL_HOLD_900_TIME + 5000) {
            // Плавно до 1200
            float progress = (float)(elapsedTime - (VERY_LOW_HOLD + INITIAL_RAMP + FINAL_HOLD_900_TIME)) / 5000;
            currentSpeed = FINAL_INITIAL_SPIN + (int)((FINAL_MEDIUM_SPIN - FINAL_INITIAL_SPIN) * progress);
        }
        else if (elapsedTime < VERY_LOW_HOLD + INITIAL_RAMP + FINAL_HOLD_900_TIME + 5000 + FINAL_HOLD_1200_TIME) {
            // Задържане на 1200
            currentSpeed = FINAL_MEDIUM_SPIN;
        }
        else if (elapsedTime < VERY_LOW_HOLD + INITIAL_RAMP + FINAL_HOLD_900_TIME + 10000 + FINAL_HOLD_1200_TIME) {
            // Плавно до целевите обороти
            float progress = (float)(elapsedTime - (VERY_LOW_HOLD + INITIAL_RAMP + FINAL_HOLD_900_TIME + 5000 + FINAL_HOLD_1200_TIME)) / 5000;
            currentSpeed = FINAL_MEDIUM_SPIN + (int)((targetSpeed - FINAL_MEDIUM_SPIN) * progress);
        }
        else {
            currentSpeed = targetSpeed;
        }
    } else if (isRinse2Spin) {
        // Специална логика за Rinse 2 spin с бавно ускорение
        if (elapsedTime < VERY_LOW_HOLD) {
            // Започваме с много ниски обороти
            currentSpeed = VERY_LOW_SPIN;
        }
        else if (elapsedTime < VERY_LOW_HOLD + SLOW_RAMP) {
            // Много бавно ускорение до 1000
            float progress = (float)(elapsedTime - VERY_LOW_HOLD) / SLOW_RAMP;
            currentSpeed = VERY_LOW_SPIN + (int)((MAX_INTERIM_SPIN - VERY_LOW_SPIN) * progress);
        }
        else {
            // Задържане на 1000
            currentSpeed = MAX_INTERIM_SPIN;
        }
    } else {
        // Логика за останалите междинни центрофуги
        if (elapsedTime < VERY_LOW_HOLD) {
            currentSpeed = VERY_LOW_SPIN;
        }
        else if (elapsedTime < VERY_LOW_HOLD + INITIAL_RAMP) {
            float progress = (float)(elapsedTime - VERY_LOW_HOLD) / INITIAL_RAMP;
            currentSpeed = (int)(INITIAL_SPIN * progress);
        }
        else if (elapsedTime < VERY_LOW_HOLD + INITIAL_RAMP + HOLD_TIME) {
            currentSpeed = INITIAL_SPIN;
        }
        else if (elapsedTime < VERY_LOW_HOLD + INITIAL_RAMP + HOLD_TIME + INITIAL_RAMP) {
            float progress = (float)(elapsedTime - (VERY_LOW_HOLD + INITIAL_RAMP + HOLD_TIME)) / INITIAL_RAMP;
            currentSpeed = INITIAL_SPIN + (int)((MEDIUM_SPIN - INITIAL_SPIN) * progress);
        }
        else if (elapsedTime < VERY_LOW_HOLD + INITIAL_RAMP + HOLD_TIME + INITIAL_RAMP + HOLD_TIME) {
            currentSpeed = MEDIUM_SPIN;
        }
        else if (elapsedTime < VERY_LOW_HOLD + INITIAL_RAMP + HOLD_TIME + INITIAL_RAMP + HOLD_TIME + INITIAL_RAMP) {
            float progress = (float)(elapsedTime - (VERY_LOW_HOLD + INITIAL_RAMP + HOLD_TIME + INITIAL_RAMP + HOLD_TIME)) / INITIAL_RAMP;
            currentSpeed = MEDIUM_SPIN + (int)((MAX_INTERIM_SPIN - MEDIUM_SPIN) * progress);
        }
        else {
            currentSpeed = MAX_INTERIM_SPIN;
        }
    }

    // Ограничаваме скоростта до целевата
    if (!isFinalSpin) {
        currentSpeed = min(currentSpeed, MAX_INTERIM_SPIN);
    } else if (currentSpeed > targetSpeed) {
        currentSpeed = targetSpeed;
    }

    // Преобразуване на оборотите в микросекунди за серво мотора
    int servoValue = SERVO_STOP + (currentSpeed / 2);
    washingDrum.writeMicroseconds(servoValue);
}

bool isPhaseComplete() {
    return (millis() - washStartTime) >= calculatePhaseTime(phaseConfigs[washPhases[currentPhase]]);
}

void handlePhaseCompletion() {
    washingDrum.writeMicroseconds(1500);
    delay(2000);
    moveToNextPhase();
}

void moveToNextPhase() {
    currentPhase++;
    washStartTime = millis();
   
    if (currentPhase >= sizeof(washPhases)/sizeof(washPhases[0])) {
        finishWashCycle();
        return;
    }
    updateDisplay();
}

void finishWashCycle() {
    isWashing = false;
    washingDrum.writeMicroseconds(1500);
    currentPhase = 0;
    
    displayFinishMessage();
    updateDisplay();
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

void displayFinishMessage() {
    oled.clearDisplay();
    oled.printInYellowSection("Finished!", 0);
    oled.printInYellowSection(":-)", 1);
    oled.sendBuffer();
    delay(3000);
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
           phaseConfigs[washPhases[currentPhase]].name);
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