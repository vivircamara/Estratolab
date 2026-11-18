#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define PRESSAO_NIVEL_MAR_HPA (1013.25)

Adafruit_BME280 bme;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Iniciando teste do BME280...");

  // Tenta o endereco padrao 0x76 e, se falhar, tenta o 0x77
  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("Erro: BME280 nao encontrado! Verifique o contato dos 4 pinos e aperte Reset.");
    while (1) delay(10);
  }

  Serial.println("BME280 conectado com sucesso!");
}

void loop() {
  Serial.print("Temperatura: ");
  Serial.print(bme.readTemperature());
  Serial.print(" *C  |  ");

  Serial.print("Umidade: ");
  Serial.print(bme.readHumidity());
  Serial.print(" %  |  ");

  Serial.print("Pressao: ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.print(" hPa  |  ");

  Serial.print("Altitude aprox.: ");
  Serial.print(bme.readAltitude(PRESSAO_NIVEL_MAR_HPA));
  Serial.println(" m");

  delay(2000);
}

//São utilizados jumpers macho-fêmea
//Relação dos pinos
//DHT 22     --------   Arduino UNO
//VCC (+)     ------    3.3V (Power) fio vermelho
//GND (-)     -----      GND (Power) fio preto
//SCL          ----       pino A5 (ANALOOG IN) fio amarelo
//SDA         ------     Pino A4 (ANALOG IN) Fio verde
