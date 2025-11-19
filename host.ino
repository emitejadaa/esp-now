#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>

const char* ssid = "IoTB";
const char* password = "inventaronelVAR";

uint8_t PEER_MAC[] = { 0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC };

WebServer server(80);

volatile bool lightOn = false;

typedef struct {
  uint8_t on;
} light_packet_t;

void onEspNowSend(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.print("ESP-NOW send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

String makePage() {
  String url = "http://" + WiFi.localIP().toString() + "/";
  String btn = lightOn ? "Apagar LED" : "Encender LED";
  String page = String(F(R"HTML(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>ESP32 Web</title>
  </head>
  <body>
    <style>
      body {
        background-color: #00012e;
        color: rgb(4, 223, 0);
        font-family: sans-serif;
        font-weight: 400;
        display: flex;
        justify-content: center;
        align-items: center;
        flex-direction: column;
        min-height: 100vh;
        gap: 12px;
      }
      button {
        background-color: rgb(4, 223, 0);
        color: #00012e;
        font-size: 20px;
        border-radius: 10px;
        padding: 10px 16px;
        border: none;
        cursor: pointer;
      }
      code { color: #7CFF7C; }
    </style>
    <h1>ESP32 HOST</h1>
    <p>Prende o apaga el relé del NODO por ESP-NOW</p>
    <p>URL de esta web: <code>)HTML")) + url + String(F(R"HTML(</code></p>
    <button id="btn">)HTML")) + btn + String(F(R"HTML(</button>
    <script>
      let state = )HTML")) + (lightOn ? "1" : "0") + String(F(R"HTML(;
      const btn = document.getElementById("btn");
      btn.addEventListener("click", async () => {
        state = 1 - state;
        btn.textContent = state ? "Apagar LED" : "Encender LED";
        await fetch('/led?on=' + (state ? 1 : 0));
      });
    </script>
  </body>
</html>
)HTML"));
  return page;
}

void handleRoot() {
  server.send(200, "text/html", makePage());
}

void handleLED() {
  if (!server.hasArg("on")) {
    server.send(400, "text/plain", "Falta argumento 'on'");
    return;
  }
  int v = server.arg("on").toInt();
  lightOn = (v == 1);

  light_packet_t pkt;
  pkt.on = lightOn ? 1 : 0;

  esp_err_t err = esp_now_send(PEER_MAC, (uint8_t*)&pkt, sizeof(pkt));
  if (err != ESP_OK) {
    Serial.printf("ESP-NOW send error: %d\n", err);
  } else {
    Serial.printf("Orden enviada -> %s\n", lightOn ? "ON" : "OFF");
  }

  server.send(200, "text/plain", String("LED ") + (lightOn ? "ON" : "OFF"));
}

void setupEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }
  esp_now_register_send_cb(onEspNowSend);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, PEER_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("No se pudo agregar el peer");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado");
  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
  Serial.printf("Web UI: http://%s/\n", WiFi.localIP().toString().c_str());

  setupEspNow();

  server.on("/", handleRoot);
  server.on("/led", handleLED);
  server.begin();
  Serial.println("Servidor iniciado");
}

void loop() {
  server.handleClient();
}
