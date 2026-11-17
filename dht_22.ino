#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println("Teste 1: DHT22 iniciado!");
}

void loop() {
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();

  if (isnan(temp) || isnan(umid)) {
    Serial.println("Falha na leitura do DHT22 - verifique os fios!");
  } else {
    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.print(" °C  |  Umidade: ");
    Serial.print(umid);
    Serial.println(" %");
  }

  delay(5000);
}

//São utilizados jumpers macho-fêmea
//Relação dos pinos
//DHT 22     --------   Arduino UNO
//VCC (+)     ------    5V (Power) fio vermelho
//GND (-)     -----      GND (Power) fio preto
//DAT          ----       2 (Digital) fio amarelo