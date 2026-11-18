#include <Wire.h>
#include <SparkFun_MMA8452Q.h>

MMA8452Q accel;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Wire.begin();

  if (accel.begin(Wire, 0x1C) == false) {
    Serial.println("Erro: GY-45 (MMA8452Q) nao encontrado!");
    while (1);
  }
}

void loop() {
  if (accel.available()) {
    Serial.print("X: ");
    Serial.print(accel.getCalculatedX(), 3);
    Serial.print(" g  |  Y: ");
    Serial.print(accel.getCalculatedY(), 3);
    Serial.print(" g  |  Z: ");
    Serial.print(accel.getCalculatedZ(), 3);
    Serial.println(" g");
  }

  delay(500);
}

//São utilizados jumpers macho-fêmea
//Relação dos pinos
// GY-45      --------   Arduino UNO
// - VCC_IN    -------  5V (Arduino Uno) fio Vermelho
// - 3.3V      -------  Sem conexão
// - GND       -------  GND (Arduino Uno) fio Preto
// - SCL       -------   A5 (Arduino Uno) fio Amarelo
// - SDA       -------   A4 (Arduino Uno) fio Verde