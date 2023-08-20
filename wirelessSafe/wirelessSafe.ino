#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

// configurations set in code
const char *ssid = "Wireless Safe";
const char *password = "password";
const char *validPassword = "1234";

WebServer server(80);

// 512 bytes EEPROM space used for storage
const int eepromSize = 512; 
const int storageMaxLength = 512;
const int eepromStorageAddress = 0;

char storedText[storageMaxLength];

void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  Serial.begin(9600);

  // set up Wi-Fi access point
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Access Point IP address: ");
  Serial.println(myIP);

  // initialize EEPROM
  EEPROM.begin(eepromSize);

  // set up HTTP server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/store", HTTP_POST, handleStore);
  server.on("/retrieve", HTTP_GET, handleRetrieve);

  server.begin();
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  // root route with a html form
  String webPage = "<html><body>";
  webPage += "<h2>Store text</h2>";
  webPage += "<form action=\"/store\" method=\"POST\">";
  webPage += "Password: <input type=\"password\" name=\"password\"><br><br>";
  webPage += "Text: <input type=\"text\" name=\"text\"><br><br>";
  webPage += "<input type=\"submit\" value=\"Submit\">";
  webPage += "</form>";
  webPage += "<h2>Retrieve text</h2>";
  webPage += "<form action=\"/retrieve\" method=\"GET\">";
  webPage += "Password: <input type=\"password\" name=\"password\"><br><br>";
  webPage += "<input type=\"submit\" value=\"Retrieve\">";
  webPage += "</form>";
  webPage += "</body></html>";
  server.send(200, "text/html", webPage);
}

void handleStore() {
  // handle text store request
  if (server.hasArg("password")) {
    String password = server.arg("password");
    if (password == validPassword) {
      if (server.hasArg("text")) {
        String text = server.arg("text");

        // null fill storage space
        for (int i = 0; i < storageMaxLength; i++) {
          storedText[i] = 0x00;
        }
        writeToEEPROM(eepromStorageAddress, storedText, storageMaxLength);
        // simple xor encrypt the text
        int length = text.length();
        for (int i = 0; i < length; i++) {
          storedText[i] = text[i] ^ validPassword[i % strlen(validPassword)];
        }
        // store encrypted text in EEPROM
        writeToEEPROM(eepromStorageAddress, storedText, length);
        server.send(200, "text/plain", "Text stored securely.");
      } else {
        server.send(400, "text/plain", "Invalid request.");
      }
    } else {
      server.send(401, "text/plain", "Unauthorized: Invalid Password");
    }
  } else {
    server.send(400, "text/plain", "Invalid request.");
  }
}
void handleRetrieve() {
  // handle text retrieve request
  if (server.hasArg("password")) {
    String password = server.arg("password");
    if (password == validPassword) {
      // read encrypted text from storage
      int length = readFromEEPROM(eepromStorageAddress, storedText, storageMaxLength);

      // decrypt the stored message
      for (int i = 0; i < length; i++) {
        storedText[i] = storedText[i] ^ validPassword[i % strlen(validPassword)];
      }

      server.send(200, "text/plain", "Stored text: " + String(storedText, length));
    } else {
      server.send(401, "text/plain", "Unauthorized: Invalid Password");
    }
  } else {
    server.send(400, "text/plain", "Invalid request.");
  }
}

void writeToEEPROM(int address, const char *data, int length) {
  // write the data into EEPROM
  for (int i = 0; i < length; i++) {
    EEPROM.write(address + i, data[i]);
  }
  EEPROM.commit();
}

int readFromEEPROM(int address, char *data, int maxLength) {
  // read data from EEPROM until null character is detected
  // and write into passed data pointer
  int length = 0;
  while (length < maxLength) {
    byte value = EEPROM.read(address + length);
    if (value == 0x00) break;
    data[length] = char(value);
    length++;
  }
  return length;
}
