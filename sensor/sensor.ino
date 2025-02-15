#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "MAX30105.h"

const char *ssid = "n2heartb_oxi";
const char *password = "n2heartb_oxi";
WiFiClient client;  // Dùng WiFiClient thay cho WiFiServer để kết nối tới server Python
WiFiUDP udp;
const int udpPort = 4210;
IPAddress serverIP(192, 168, 1, 27);  // IP của server Python (đã biết hoặc nhận từ UDP)
const int serverPort = 8882;          // Cổng TCP của server Python

MAX30105 particleSensor;
const int maxBufferSize = 100;
long irBuffer[maxBufferSize];
long redBuffer[maxBufferSize];
long greenBuffer[maxBufferSize];
int bufferIndex = 0;

int currentBufferSize = 100;
unsigned long lastSendTime = 0;

void discoverServer() {
  udp.begin(udpPort);
  Serial.println("Broadcasting discovery message...");

  udp.beginPacket(IPAddress(255, 255, 255, 255), udpPort);
  udp.print("DISCOVER_SERVER");
  udp.endPacket();

  unsigned long startTime = millis();
  while (millis() - startTime < 3000) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
      char buffer[32];
      udp.read(buffer, packetSize);
      buffer[packetSize] = '\0';
      if (String(buffer).startsWith("SERVER_IP:")) {
        serverIP.fromString(String(buffer).substring(10));
        Serial.print("Found Server at: ");
        Serial.println(serverIP);
        break;
      }
    }
  }
  udp.stop();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());

  // Kiểm tra nếu kết nối được tới IP mặc định
  if (serverIP == IPAddress(0, 0, 0, 0)) {
    Serial.println("Can't connect to default server, finding...");
    discoverServer();  // Nếu không kết nối, tìm server qua UDP
  }
  Serial.println("Connected to server!");

  // Kết nối tới server Python qua TCP
  if (client.connect(serverIP, serverPort)) {
    Serial.println("Connected to server.");
  } else {
    Serial.println("Connection to server failed.");
  }

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 not found!");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x0A); // Set IR LED amplitude
  particleSensor.setPulseAmplitudeGreen(0x0A); // Set Green LED amplitude
}

void handleServerCommunication() {
  // Gửi tín hiệu từ cả 3 kênh đến server
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();
  long greenValue = particleSensor.getGreen();
  
  irBuffer[bufferIndex] = irValue;
  redBuffer[bufferIndex] = redValue;
  greenBuffer[bufferIndex] = greenValue;
  bufferIndex++;

  // Khi buffer đủ size, gửi dữ liệu đến server dưới dạng một chuỗi mỗi giây
  if (bufferIndex >= currentBufferSize) {
    String dataToSend = "";  // Tạo chuỗi dữ liệu để gửi

    for (int i = 0; i < currentBufferSize; i++) {
      dataToSend += String(irBuffer[i], HEX) + "," + String(redBuffer[i], HEX) + "," + String(greenBuffer[i], HEX);
      if (i < currentBufferSize - 1) {
        dataToSend += ",";  // Dùng dấu phẩy phân cách các giá trị
      }
    }

    client.print(dataToSend);  // Gửi dữ liệu dưới dạng một chuỗi
    Serial.println("Data sent to server:");
    Serial.println(dataToSend);

    bufferIndex = 0;  // Reset lại chỉ số buffer sau khi gửi dữ liệu
  }

  delay(10);  // Giảm thời gian delay để thu thập nhanh hơn (tần suất lấy mẫu ~100Hz)
}

void loop() {
  if (client.connected()) {
    handleServerCommunication();
  } else {
    Serial.println("Disconnected from server. Attempting to reconnect...");
    // Nếu mất kết nối, thử kết nối lại với server
    if (client.connect(serverIP, serverPort)) {
      Serial.println("Reconnected to server.");
      client.print("ping");  // Gửi ping lại khi kết nối lại
    }
  }
}
