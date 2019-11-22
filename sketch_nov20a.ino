#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define ORANGE_PIN 2
#define YELLOW_PIN 18

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
WiFiServer server(80);
String header;

int counter = 0;
int shots = 0;
int interval = 0;
int current_shots = 0;
bool state = 0;

unsigned long timer = 0;
unsigned long int previous_time = 0;
unsigned long int current_time = 0;

uint8_t hours = 0;
uint8_t minutes = 0;
uint8_t seconds = 0;
uint8_t duration = 0;

uint8_t v_hours = 0;
uint8_t v_minutes = 0;
uint8_t v_seconds = 0;

const char* ssid = "iPhone";
const char* password = "12345678910";

void get_parameters(String req);
void take_picture();
void display_info();
void handle_requests();
void reduce_time();
void calculate_duration();

void setup() {
  pinMode(ORANGE_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(OLED_RST, OUTPUT);
  
  digitalWrite(ORANGE_PIN, HIGH);
  digitalWrite(YELLOW_PIN, HIGH);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) {
    for(;;);
  }

  display.clearDisplay();
  display.setRotation(2); 
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Not connected");
  display.display();

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
  }

  server.begin();
}

void loop() {
	take_picture();
	display_info();
	handle_requests();

	previous_time = current_time;
}

void get_parameters(String req) {
  int counter = 0;
  int shots_temp = 0;
  int interval_temp = 0;
  
  for (int i = 0; i < req.length(); i++) {
    if (req[i] == '=' && counter == 0) {
        int j = i + 1;
        while(req[j] != '&') {
            shots_temp *= 10;
            shots_temp += req[j] - '0';
            j++;
        }
        shots = shots_temp;
        counter = 1;
    } else if (req[i] == '=' && counter == 1){
        int k = i + 1;
        while(req[k] != '%') {
           interval_temp *= 10;
           interval_temp += req[k] - '0';
           k++;
        } 
        interval = interval_temp;
    }
  }
}

void take_picture() {
  if (state && current_shots < shots) {
      current_time = millis();
      timer += current_time - previous_time;
      if (timer >= interval) {
        digitalWrite(YELLOW_PIN, LOW);
        digitalWrite(ORANGE_PIN, LOW);
        delay(10);
        digitalWrite(YELLOW_PIN, HIGH);
        digitalWrite(ORANGE_PIN, HIGH); 
        timer = 0;
        current_shots++;
      } else {
        digitalWrite(YELLOW_PIN, HIGH);
        digitalWrite(ORANGE_PIN, HIGH);
      }
  } else {
    timer = 0;
    current_time = 0;
    previous_time = 0;  
  }
}

void display_info() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("IPv4: ");
  display.println(WiFi.localIP());
  display.print("Shots: ");
  display.print(current_shots);
  display.print("/");
  display.println(shots);
  display.print("Interval: ");
  display.print(interval/1000);
  display.println("s");
  display.print("Shot due in: ");
  display.print((interval - timer) / 1000);
  display.println("s");
	reduce_time();
	display.print("Total time: ");
	display.print(hours);
	display.print(":");
	display.print(minutes);
	display.print(":");
	display.println(seconds);
	calculate_duration();
	display.print("Video length: ");
	display.print(v_hours);
	display.print(":");
	display.print(v_minutes);
	display.print(":");
	display.print(v_seconds);
  
	display.display();
}

void handle_requests() {
  WiFiClient client = server.available();

  if (client) {
    String current_line = "";
    while(client.connected()) {
      if (client.available()) {
        char c = client.read();
        header += c;
        if (c == '\n') {
          if (current_line.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
  
            if (header.indexOf("GET /start") >= 0) {
              get_parameters(header);
              current_shots = 0;
              state = 1;
              timer = 0;
            } else if (header.indexOf("GET /stop") >= 0) {
              current_shots = 0;
              state = 0;
              timer = 0;
							hours = 0;
							minutes = 0;
							seconds = 0;
            }

            client.println("<!DOCTYPE html><html> <head> <title>Shtrakalka</title> </head> <body> <p>Number of Shots:</p> <input type=\"text\" name=\"shots\" id=\"shots\" value=\"100\"><br> <p>Interval:</p> <input type=\"text\" name=\"interval\" id=\"interval\" value=\"10\"><br> <br><br><button id=\"start_button\">Start</button> <button id=\"stop_button\">Stop</button> <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js\"></script> <script> $('#start_button').click(function(e) { e.preventDefault(); shots = $('#shots').val(); interval = $('#interval').val() * 1000; $.get('/start?shots=' + shots + '&interval=' + interval + '%', (data) => { console.log(data); }) }); $('#stop_button').click(function (e) { e.preventDefault(); $.get('/stop', (data) => { console.log(data); }) }); </script> </body></html>");
            client.println();
            break;
          } else {
            current_line = "";  
          } 
        } else if (c != '\r') {
            current_line += c;
        }
      }  
    }
    header = "";
    client.stop();
  }
}

void reduce_time() {
	int	_time = shots * (interval / 1000);

	hours = _time / 3600;
	_time -= hours * 3600;
	minutes = _time / 60;
	seconds = _time - minutes * 60;
}

void calculate_duration() {
	int _time = shots / 24;

	v_hours = _time / 3600;
	_time -= v_hours * 3600;
	v_minutes = _time / 60;
	v_seconds = _time - v_minutes * 60;
}