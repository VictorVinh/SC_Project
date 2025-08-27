#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdlib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- THÔNG TIN CẦN THAY ĐỔI ---
const char* ssid = "TEN_WIFI_CUA_BAN";         // <-- THAY TÊN WIFI
const char* password = "MAT_KHAU_WIFI_CUA_BAN"; // <-- THAY MẬT KHẨU WIFI
const char* server_ip = "192.168.1.10"; // <-- THAY BẰNG ĐỊA CHỈ IP CỦA MÁY TÍNH

// --- BIẾN LƯU THÔNG TIN NGƯỜI CHƠI (SẼ ĐƯỢC NHẬP TỪ SERIAL) ---
String playerName;
String playerClass;
// ---------------------------------

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define RIGHT 0
#define UP    1
#define LEFT  2
#define DOWN  3

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Game variables
int score = 0;
int level = 1;
int gamespeed = 150;
bool isGameOver = false;
int dir;
const int buzzerPin = 18;

struct FOOD { int x; int y; int yes; };
struct SNAKE { int x[200]; int y[200]; int node; int dir; };

FOOD food;
SNAKE snake;

// --- HÀM MỚI: NHẬP THÔNG TIN NGƯỜI CHƠI ---
void getPlayerInfoFromSerial() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Nhập tên
  display.setCursor(0, 0);
  display.println("Open Serial Monitor");
  display.println("to enter player info.");
  display.display();

  Serial.println("\n\nVui long nhap TEN cua ban roi an Enter:");
  while (!Serial.available()) { delay(100); } // Đợi người dùng nhập
  playerName = Serial.readStringUntil('\n');
  playerName.trim(); // Xóa ký tự thừa
  Serial.print("Ten da luu: ");
  Serial.println(playerName);

  // Nhập lớp
  Serial.println("\nVui long nhap LOP cua ban roi an Enter:");
  while (!Serial.available()) { delay(100); } // Đợi người dùng nhập
  playerClass = Serial.readStringUntil('\n');
  playerClass.trim(); // Xóa ký tự thừa
  Serial.print("Lop da luu: ");
  Serial.println(playerClass);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Player: ");
  display.println(playerName);
  display.print("Class: ");
  display.println(playerClass);
  display.display();
  delay(2000);
}

// --- HÀM GIAO TIẾP MẠNG (đã cập nhật) ---
void sendPlayerData(const char* status, int currentScore) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = "http://" + String(server_ip) + ":3000/player";

    http.begin(serverPath);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> doc;
    doc["name"] = playerName;      // Sử dụng biến đã nhập
    doc["class"] = playerClass;    // Sử dụng biến đã nhập
    doc["status"] = status;
    doc["score"] = currentScore;

    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    } else {
      Serial.printf("Error code: %d\n", httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected. Cannot send data.");
  }
}

// --- CÁC HÀM LOGIC GAME (không thay đổi, giữ nguyên như trước) ---
const uint8_t ele[] PROGMEM = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, };
void element(int x, int y) { display.drawBitmap(x, y, ele, 8, 8, 1); }
void buttonUpISR() { dir = UP; }
void buttonDownISR() { dir = DOWN; }
void buttonLeftISR() { dir = LEFT; }
void buttonRightISR() { dir = RIGHT; }
void generateFood();
void snakeGame() {
  switch (snake.dir) {
    case RIGHT: snake.x[0] += 8; if(snake.x[0] > 120) snake.x[0] = 0; break;
    case UP:    snake.y[0] -= 8; if(snake.y[0] < 0) snake.y[0] = 56; break;
    case LEFT:  snake.x[0] -= 8; if(snake.x[0] < 0) snake.x[0] = 120; break;
    case DOWN:  snake.y[0] += 8; if(snake.y[0] > 56) snake.y[0] = 0; break;
  }
  if ((snake.x[0] == food.x) && (snake.y[0] == food.y)) {
    snake.node++;
    score += 5;
    level = score / 20 + 1;
    tone(buzzerPin, 1000, 100);
    generateFood();
  }
  for (int i = 1; i < snake.node; i++) {
    if(snake.x[0] == snake.x[i] && snake.y[0] == snake.y[i]) {
      isGameOver = true;
    }
  }
  for (int i = snake.node - 1; i > 0; i--) {
    snake.x[i] = snake.x[i - 1];
    snake.y[i] = snake.y[i - 1];
  }
}
void key() {
  if (dir == DOWN && snake.dir != UP) snake.dir = DOWN;
  if (dir == RIGHT && snake.dir != LEFT) snake.dir = RIGHT;
  if (dir == LEFT && snake.dir != RIGHT) snake.dir = LEFT;
  if (dir == UP && snake.dir != DOWN) snake.dir = UP;
}
bool isFoodOnSnake() {
  for (int i = 0; i < snake.node; i++) {
    if (food.x == snake.x[i] && food.y == snake.y[i]) return true;
  }
  return false;
}
void generateFood() {
  do {
    food.x = random(0, 16) * 8;
    food.y = random(0, 8) * 8;
  } while (isFoodOnSnake());
}
void displaySnake(){
  for (int i = 0; i < snake.node; i++) element(snake.x[i], snake.y[i]);
}
void resetGame() {
    isGameOver = false;
    score = 0;
    level = 1;
    snake.x[0] = 64; snake.y[0] = 32;
    snake.x[1] = 56; snake.y[1] = 32;
    snake.dir = RIGHT;
    snake.node = 2;
    dir = RIGHT;
    generateFood();
}
void gameOverScreen() {
  tone(buzzerPin, 200, 500);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(15, 10);
  display.println("Game Over");
  display.setTextSize(1);
  display.setCursor(30, 40);
  display.print("Score: ");
  display.println(score);
  display.display();
  Serial.println("Game Over! Sending final score...");
  sendPlayerData("da choi xong", score);
  delay(5000);
}

// --- HÀM SETUP VÀ LOOP CHÍNH (đã cập nhật) ---
void setup() {
  Serial.begin(115200);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Connecting to WiFi...");
  display.println(ssid);
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("WiFi Connected!");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);

  // --- GỌI HÀM NHẬP LIỆU ---
  getPlayerInfoFromSerial();
  // -------------------------

  pinMode(15, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(5), buttonUpISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(2), buttonDownISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(15), buttonLeftISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(4), buttonRightISR, FALLING);

  randomSeed(analogRead(0));
}

void loop() {
  resetGame();

  display.clearDisplay();
  display.setCursor(20, 20);
  display.setTextSize(1);
  display.println("Starting game...");
  display.display();
  Serial.println("New game starting. Sending 'playing' status...");
  sendPlayerData("dang choi", 0);
  delay(1000);

  while(!isGameOver) {
    display.clearDisplay();
    displaySnake();
    element(food.x, food.y);
    display.display();
    
    key();
    snakeGame();
    delay(gamespeed);
  }

  gameOverScreen();
}
