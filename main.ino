#include <WiFi.h>
#include <WebServer.h>
#include <IRremote.hpp>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Preferences.h>
#include <vector>

#define IR_RECEIVE_PIN 14
#define IR_SEND_PIN 15
#define TFT_CS    15
#define TFT_RST   4
#define TFT_DC    2

const char* ssid = "Wokwi-GUEST";
const char* password = "";

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
WebServer server(80);
Preferences preferences;

std::vector<uint32_t> irCodeMap[10]; // Use a vector array to map digits to IR codes

class SongRequest {
public:
  String song;
  String customerId;

  SongRequest(String s, String c) : song(s), customerId(c) {}
};

std::vector<SongRequest> songQueue;
std::vector<String> customers;
char currentNumber = '0';
bool recordMode = true;
bool playMode = false;

unsigned long interval = 0; // 自动发送的间隔时间，单位为毫秒
unsigned long previousMillis = 0; // 上一次发送的时间

// HTML content for admin and customer pages
const char ADMIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Admin Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; }
    h1 { color: #333; }
    p { font-size: 1.2em; }
    a { display: inline-block; padding: 10px 20px; margin: 10px; border: 1px solid #333; color: #333; text-decoration: none; border-radius: 4px; }
    a:hover { background-color: #333; color: #fff; }
    input { padding: 10px; font-size: 1em; }
  </style>
</head>
<body>
  <h1>Admin Control</h1>
  <p>Current Mode: %MODE%</p>
  <p>Current Interval: %INTERVAL% seconds</p>
  <a href="/toggle">Toggle Mode</a><br><br>
  <h2>Song Queue</h2>
  <ul>
    %SONG_QUEUE%
  </ul>
  <h2>Set Auto Send Interval</h2>
  <form action="/setInterval" method="POST">
    <label for="interval">Interval (ms):</label>
    <input type="text" id="interval" name="interval"><br><br>
    <input type="submit" value="Set Interval">
  </form>
  <h2>Add Customer</h2>
  <form action="/addCustomer" method="POST">
    <label for="customer">Customer ID:</label>
    <input type="text" id="customer" name="customer"><br><br>
    <input type="submit" value="Add Customer">
  </form>
</body>
</html>
)rawliteral";

const char CUSTOMER_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Customer Page</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; }
    h1 { color: #333; }
    p { font-size: 1.2em; }
    img { width: 200px; height: 200px; }
  </style>
</head>
<body>
  <h1>Welcome, Customer %CUSTOMER_ID%</h1>
  <p>Scan this QR code to submit your song:</p>
  <img src="https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=%URL%">
  <p>GO to site:</p>
  <a href="%URL%">Submit your song</a>
</body>
</html>
)rawliteral";

const char CUSTOMER_SUBMIT_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Submit Song</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; }
    h1 { color: #333; }
    form { margin: 20px auto; }
    input[type="text"] { padding: 10px; font-size: 1.2em; }
    input[type="submit"] { padding: 10px 20px; font-size: 1.2em; }
  </style>
</head>
<body>
  <h1>Submit Song</h1>
  <form action="/submitSong" method="POST">
    <label for="song">Song Number:</label>
    <input type="text" id="song" name="song"><br><br>
    <input type="hidden" id="customer" name="customer" value="%CUSTOMER_ID%">
    <input type="submit" value="Submit">
  </form>
</body>
</html>
)rawliteral";

// Function declarations
void handleAdmin();
void handleCustomer();
void handleAddCustomer();
void handleToggle();
void handleSubmit();
void handleSubmitSong();
void handleSetInterval();
void displayCurrentNumber();
void displayMapping(char number, uint32_t code);
void sendIRCode(String song);
void blinkLED(int pin, int times);
void displayQRCode(String url);
void loadPreferences();
void savePreferences();
bool allNumbersMapped();
void updateAdminPage();
void updateTFTScreen();

void handleAdmin() {
  String html = FPSTR(ADMIN_PAGE);
  html.replace("%MODE%", playMode ? "Play Mode" : "Record Mode");

  String songList = "";
  for (const auto& song : songQueue) {
    songList += "<li>Song: " + song.song + " (Customer: " + song.customerId + ")</li>";
  }
  html.replace("%SONG_QUEUE%", songList);
  html.replace("%INTERVAL%", String(interval / 1000)); // 以秒为单位显示当前间隔时间

  server.send(200, "text/html", html);
}

void updateAdminPage() {
  String html = FPSTR(ADMIN_PAGE);
  html.replace("%MODE%", playMode ? "Play Mode" : "Record Mode");

  String songList = "";
  for (const auto& song : songQueue) {
    songList += "<li>Song: " + song.song + " (Customer: " + song.customerId + ")</li>";
  }
  html.replace("%SONG_QUEUE%", songList);
  html.replace("%INTERVAL%", String(interval / 1000)); // 以秒为单位显示当前间隔时间

  server.sendContent(html);
}

void updateTFTScreen() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  if (songQueue.empty()) {
    tft.print("No songs in queue");
  } else {
    for (const auto& song : songQueue) {
      tft.print("Song: " + song.song + " from " + song.customerId);
      tft.setCursor(0, tft.getCursorY() + 20);
    }
  }
}

void handleCustomer() {
  if (server.hasArg("customer")) {
    String customerId = server.arg("customer");
    String html = FPSTR(CUSTOMER_PAGE);
    html.replace("%CUSTOMER_ID%", customerId);
    String url = "http://localhost:9080/submit?customer=" + customerId;
    html.replace("%URL%", url);
    String link = "<a href=\"" + url + "\">Submit your song</a>";
    html.replace("%LINK%", link);

    server.send(200, "text/html", html);
    displayQRCode(url);
    
  } else {
    server.send(400, "text/html", "<html><body><h1>Error: No customer ID provided</h1><a href='/admin'>Back</a></body></html>");
  }
}

void handleAddCustomer() {
  if (server.hasArg("customer")) {
    String customerId = server.arg("customer");
    customers.push_back(customerId);
    handleCustomer();
  } else {
    server.send(400, "text/html", "<html><body><h1>Error: No customer ID provided</h1><a href='/admin'>Back</a></body></html>");
  }
}

void handleToggle() {
  playMode = !playMode;
  handleAdmin();
  if (playMode) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.print("Play mode on");
  } else {
    displayCurrentNumber();
  }
}

void handleSubmit() {
  if (server.hasArg("customer")) {
    String customerId = server.arg("customer");
    String html = FPSTR(CUSTOMER_SUBMIT_PAGE);
    html.replace("%CUSTOMER_ID%", customerId);
    server.send(200, "text/html", html);
  } else {
    server.send(400, "text/html", "<html><body><h1>Error: No customer ID provided</h1><a href='/admin'>Back</a></body></html>");
  }
}

void handleSubmitSong() {
  if (server.hasArg("song") && server.hasArg("customer")) {
    String song = server.arg("song");
    String customerId = server.arg("customer");
    // 检查歌曲号码是否为六位数
    if (song.length() != 6 || !song.toInt()) {
      server.send(400, "text/html", "<html><body><h1>Error: Song number must be a six-digit number</h1><a href='/submit?customer=" + customerId + "'>Back</a></body></html>");
      return;
    }
    songQueue.push_back(SongRequest(song, customerId));
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.print("Song queued: ");
    tft.print(song);
    tft.setCursor(0, 20);
    tft.print("Customer: ");
    tft.print(customerId);
    tft.setCursor(0, 40);
    for (const auto& songs : songQueue) {
      tft.print("Song: " + songs.song + " from " + songs.customerId);
      tft.setCursor(0, tft.getCursorY() + 20);
    }
    savePreferences();
    server.send(200, "text/html", "<html><body><h1>Song Queued!</h1><a href='/submit?customer=" + customerId + "'>Back</a></body></html>");
  } else {
    server.send(400, "text/html", "<html><body><h1>Error: No song number provided</h1><a href='/customer?customer=" + server.arg("customer") + "'>Back</a></body></html>");
  }
}

void handleSetInterval() {
  if (server.hasArg("interval")) {
    interval = server.arg("interval").toInt();
    server.send(200, "text/html", "<html><body><h1>Interval Set!</h1><a href='/admin'>Back to Admin</a></body></html>");
  } else {
    server.send(400, "text/html", "<html><body><h1>Error: No interval provided</h1><a href='/admin'>Back to Admin</a></body></html>");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing IR Receiver...");
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR Receiver initialized.");

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  displayCurrentNumber();
  Serial.println("ILI9341 initialized.");

  pinMode(IR_SEND_PIN, OUTPUT);
  digitalWrite(IR_SEND_PIN, LOW);

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    attempt++;
    if (attempt > 50) {
      Serial.println("\nFailed to connect to WiFi. Restarting...");
      ESP.restart();
    }
  }
  Serial.println(" Connected!");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  preferences.begin("songQueue", false);
  loadPreferences();

  server.on("/", handleAdmin);
  server.on("/admin", handleAdmin);
  server.on("/toggle", handleToggle);
  server.on("/addCustomer", HTTP_POST, handleAddCustomer);
  server.on("/customer", handleCustomer);
  server.on("/submit", handleSubmit);
  server.on("/submitSong", HTTP_POST, handleSubmitSong);
  server.on("/setInterval", HTTP_POST, handleSetInterval);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();

  if (!playMode && IrReceiver.decode()) {
    Serial.println("IR signal received.");
    uint32_t irCode = IrReceiver.decodedIRData.decodedRawData;
    irCodeMap[currentNumber - '0'].push_back(irCode); // Store IR code in the appropriate vector
    Serial.print("Recorded ");
    Serial.print(currentNumber);
    Serial.print(" for code ");
    Serial.println(irCode, HEX);
    displayMapping(currentNumber, irCode);
    IrReceiver.resume();
  }

  if (playMode && !songQueue.empty()) {
    unsigned long currentMillis = millis();
    if (interval > 0 && currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      String song = songQueue.front().song;
      String customerId = songQueue.front().customerId;
      sendIRCode(song);
      songQueue.erase(songQueue.begin()); // 删除队列中的第一个元素
      savePreferences(); // 保存首选项
      Serial.print("Sent and deleted song number: ");
      Serial.print(song);
      Serial.print(" from customer ID: ");
      Serial.println(customerId);
      updateAdminPage(); // 更新Admin页面
      updateTFTScreen(); // 更新TFT屏幕
    }
  }
}

void displayCurrentNumber() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.print("Mapping signal to:");
  tft.setCursor(0, 40);
  tft.setTextSize(4);
  tft.print(currentNumber);
  Serial.print("Current number to map: ");
  Serial.println(currentNumber);
}

void displayMapping(char number, uint32_t code) {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.print("Mapped ");
  tft.print(number);
  tft.print(" to code:");
  tft.setCursor(0, 40);
  tft.setTextSize(4);
  tft.print(code, HEX);
  delay(2000);

  // 检查所有数字是否完成输入
  if (allNumbersMapped()) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(4);
    tft.print("Finish");
  } else {
    currentNumber = (currentNumber == '9') ? '0' : currentNumber + 1;
    displayCurrentNumber();
  }
}

bool allNumbersMapped() {
  for (int i = 0; i < 10; i++) {
    if (irCodeMap[i].empty()) {
      return false;
    }
  }
  return true;
}

void sendIRCode(String song) {
  for (char digit : song) {
    uint32_t irCode = 0;
    for (uint32_t code : irCodeMap[digit - '0']) {
      irCode = code;
      break;
    }
    if (irCode != 0) {
      blinkLED(IR_SEND_PIN, 32);
      delay(500);
    }
  }
}

void blinkLED(int pin, int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(560);
    digitalWrite(pin, LOW);
    delayMicroseconds(560);
  }
}

void displayQRCode(String url) {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.print("Scan QR Code:");
  tft.setCursor(0, 40);
  tft.setTextSize(2);
  tft.print(url);
}

void loadPreferences() {
  int size = preferences.getInt("queue_size", 0);
  songQueue.clear();
  for (int i = 0; i < size; i++) {
    String songKey = "song_" + String(i);
    String customerKey = "customer_" + String(i);
    String song = preferences.getString(songKey.c_str(), "");
    String customer = preferences.getString(customerKey.c_str(), "");
    if (song.length() > 0 && customer.length() > 0) {
      songQueue.push_back(SongRequest(song, customer));
    }
  }
}

void savePreferences() {
  preferences.putInt("queue_size", songQueue.size());
  for (int i = 0; i < songQueue.size(); i++) {
    String songKey = "song_" + String(i);
    String customerKey = "customer_" + String(i);
    preferences.putString(songKey.c_str(), songQueue[i].song);
    preferences.putString(customerKey.c_str(), songQueue[i].customerId);
  }
}
