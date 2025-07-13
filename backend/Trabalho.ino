#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include <ESP32Servo.h>
#include <SolarCalculator.h>
#include "esp_wifi.h"
#include "esp_timer.h"

#define PWMSERVO 15
#define RELE 18

// Configurações de WiFi
const char* ssid = "iP";
const char* password = "dc99gf9sh39jx";

// Configurações do EMQX
const char* mqtt_server = "f98b1789.ala.dedicated.azure.emqxcloud.com";
const int mqtt_port = 8883;
const char* mqtt_user = "admin";
const char* mqtt_password = "Admin@";
const char* mqtt_client_id = "ESP32_Client_SSL_Teste";

// Tópicos MQTT
const char* topic_subscribe = "esp32/comando";
const char* topic_publish = "esp32/angulo";

// variaveis
short int angulo = 0;
short int anguloAnterior = 1;
bool manual = true;
Servo servo;
float latitude = -19.9167;
float longitude = -43.9345; // bh
float sunriseAzimuth = 0;
float sunsetAzimuth = 0;
float azimuteAtual = 0;
bool novoDia = true;
float offset = 0;
// esp_timer_handle_t wakeTimer;
// volatile bool wakeupFromTimer = true;

// Certificado CA (Root Certificate) do servidor EMQX
const char* ca_cert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// Objetos WiFi e MQTT
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Variáveis de controle
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

// funcao para determinar se a string recebida e valida
bool parseAngle(String str, short int &angle) {
  // Verifica se a string está vazia
  if (str.length() == 0) return false;

  // Verifica se todos os caracteres são dígitos (sem sinal negativo)
  for (int i = 0; i < str.length(); i++) {
    if (!isDigit(str[i])) return false;
  }

  int value = str.toInt();
  if (value < 0 || value > 180) return false;

  angle = value;
  return true;
}

// void IRAM_ATTR onWakeTimer(void* arg) {
//   wakeupFromTimer = true;
// }

// Função 1: Azimute atual
double getCurrentAzimuth(time_t utc) {
  double azimuth, elevation;
  calcHorizontalCoordinates(utc, latitude, longitude, azimuth, elevation);
  return azimuth;
}

// Função 2: Azimute do nascer do sol
double getSunriseAzimuth(time_t utc) {
  double azimuth, elevation;
  // double transit, sunrise, sunset;
  // calcSunriseSunset(utc, latitude, longitude, transit, sunrise, sunset);
  // time_t sunriseTime = floor(sunrise * 3600);  // Converter h decimal para segundos
  // Montar hora UTC manualmente (ex: 9 de julho de 2025 às 12:00 UTC)
  struct tm tm_utc = { 0 };
  tm_utc.tm_year = 2025 - 1900;   // ano - 1900
  tm_utc.tm_mon  = 7 - 1;         // mês de 0 a 11
  tm_utc.tm_mday = 9;
  tm_utc.tm_hour = 6 + 3;
  tm_utc.tm_min  = 31;
  tm_utc.tm_sec  = 0;
  time_t utcManual = mktime(&tm_utc);
  calcHorizontalCoordinates(utcManual, latitude, longitude, azimuth, elevation);
  return azimuth;
}

// Função 3: Azimute do pôr do sol
double getSunsetAzimuth(time_t utc) {
  double azimuth, elevation;
  // double transit, sunrise, sunset;
  // calcSunriseSunset(utc, latitude, longitude, transit, sunrise, sunset);
  // time_t sunsetTime = floor(sunset * 3600);
  struct tm tm_utc = { 0 };
  tm_utc.tm_year = 2025 - 1900;   // ano - 1900
  tm_utc.tm_mon  = 7 - 1;         // mês de 0 a 11
  tm_utc.tm_mday = 9;
  tm_utc.tm_hour = 17 + 3;
  tm_utc.tm_min  = 31;
  tm_utc.tm_sec  = 0;
  time_t utcManual = mktime(&tm_utc);
  calcHorizontalCoordinates(utcManual, latitude, longitude, azimuth, elevation);
  return azimuth;
}

// Configurar hora (necessário para validação SSL)
void setClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Aguardando sincronização NTP");
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print("Hora atual: ");
  Serial.print(asctime(&timeinfo));
}

// Função callback para mensagens MQTT recebidas
void callback(char* topic, byte* payload, unsigned int length) {
  // wakeupFromTimer = true;
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Processar comandos recebidos
  if (message == "M") {
    manual = 1;
  }
  else if (message == "A") {
    manual = 0;
  }
  else {
    parseAngle(message, angulo);
  }
}

// Conectar ao WiFi
void setupWifi() {
  Serial.println("\nConectando a " + String(ssid));

  // Inicia o Wi-Fi em modo STA
  WiFi.mode(WIFI_STA);

  // Cria uma estrutura de configuração
  wifi_config_t wifi_config;
  
  // Limpa a estrutura para evitar lixo de memória
  memset(&wifi_config, 0, sizeof(wifi_config));

  // Copia o SSID e a senha para a estrutura de configuração
  strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
  strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

  // Define o listen_interval
  // O valor é em unidades de Beacon Interval (geralmente ~100ms)
  // 150 * 100ms = 15000ms = 15 segundos
  wifi_config.sta.listen_interval = 150;

  // Aplica a configuração ao Wi-Fi do ESP32
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

  // Inicia o processo de conexão
  WiFi.begin();

  // O loop de espera pela conexão permanece o mesmo
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
}

// Reconectar ao MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    
    // Tentar conectar
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("conectado");
      
      // Publicar mensagem de conexão
      client.publish(topic_publish, "0");
      
      // Subscrever ao tópico de comandos
      client.subscribe(topic_subscribe);
      Serial.print("Inscrito no tópico: ");
      Serial.println(topic_subscribe);
    }
    else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELE, OUTPUT);
  digitalWrite(RELE, LOW);
  
  // Conectar ao WiFi
  setupWifi();
  
  // Sincronizar relógio (necessário para SSL)
  setClock();
  
  // Configurar certificado CA
  espClient.setCACert(ca_cert);
  
  // Configurar servidor MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // Conectar ao MQTT
  reconnect();

  // Configurar Power Management - permite que o wifi fique em modo de economia de energia quando o esp32 esta em sleep
  esp_pm_config_esp32_t pm_config = {
      .max_freq_mhz = 160,
      .min_freq_mhz = 80,
      .light_sleep_enable = true
  };
  ESP_ERROR_CHECK(esp_pm_configure(&pm_config))

  // Configura modo de economia: mantém Wi-Fi conectado mas rádio dormindo
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);  // Garante wake via Wi-Fi

  servo.setPeriodHertz(50); // 50Hz
  servo.attach(PWMSERVO, 500, 2400); // Largura do pulso (duty cycle): varia entre ~500 µs e ~2400 µs, ideal para sg90

  // const esp_timer_create_args_t timer_args = {
  //   .callback = &onWakeTimer,
  //   .arg = nullptr,
  //   .name = "WakeTimer"
  // };
  // esp_timer_create(&timer_args, &wakeTimer);
  // esp_timer_start_periodic(wakeTimer, 100000000); // 100 s

  // esp_sleep_enable_timer_wakeup(50000000); // 50 s
}

void loop() {
  // Manter conexão MQTT ativa
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
    // wakeupFromTimer = false;
    time_t utcNow = time(nullptr);
    if (novoDia) {
      sunriseAzimuth = getSunriseAzimuth(utcNow);
      sunsetAzimuth = getSunsetAzimuth(utcNow);
      Serial.println("=== Posição do Sol ===");
      Serial.printf("Azimute do nascer do sol: %.2f°\n", sunriseAzimuth);
      Serial.printf("Azimute do pôr do sol: %.2f°\n", sunsetAzimuth);
      offset = (180 - ((360 - sunriseAzimuth) + sunsetAzimuth)) / 2;
      novoDia = false;
    }
    azimuteAtual = getCurrentAzimuth(utcNow);
    if (!manual) {
      if (azimuteAtual > sunsetAzimuth || azimuteAtual < sunriseAzimuth) {
        angulo = 0;
        novoDia = true;
      }
      else {
        if (angulo < 180) {
          angulo = azimuteAtual + (360 - sunriseAzimuth) + offset;
        }
        else angulo = azimuteAtual - sunriseAzimuth + offset;
      }
    }
    if (angulo != anguloAnterior) {
      digitalWrite(RELE, HIGH);
      servo.write(angulo);
      String info = String(angulo) + ";" + String(azimuteAtual) + ";" + String(manual) + ";" + String(digitalRead(RELE));
      client.publish(topic_publish, info.c_str());
      delay(3000);
      digitalWrite(RELE, LOW);
      anguloAnterior = angulo;
    }

    String info = String(angulo) + ";" + String(azimuteAtual) + ";" + String(manual) + ";" + String(digitalRead(RELE));
    client.publish(topic_publish, info.c_str());

    esp_light_sleep_start();
}
