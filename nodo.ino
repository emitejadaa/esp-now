#include <WiFi.h>
#include <esp_now.h>

const char* ssid = "IoTB";
const char* password = "inventaronelVAR";

const int LED_PIN = 2;

typedef struct {
  uint8_t on;
} light_packet_t;

void onEspNowRecv(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  if (len != sizeof(light_packet_t)) return;

  light_packet_t pkt;
  memcpy(&pkt, incomingData, sizeof(pkt));

  digitalWrite(LED_PIN, pkt.on ? HIGH : LOW);

  if (info && info->src_addr) {
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            info->src_addr[0], info->src_addr[1], info->src_addr[2],
            info->src_addr[3], info->src_addr[4], info->src_addr[5]);
    Serial.printf("Orden %s de %s\n", pkt.on ? "ON" : "OFF", macStr);
  } else {
    Serial.printf("Orden %s\n", pkt.on ? "ON" : "OFF");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando NODO al AP");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nNODO conectado al AP");
  Serial.print("MAC del NODO: ");
  Serial.println(WiFi.macAddress());
  Serial.printf("Canal actual: %d\n", WiFi.channel());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onEspNowRecv);
}

void loop() {
}
