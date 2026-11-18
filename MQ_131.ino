const int PINO_MQ131 = A0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
}

void loop() {
  int leituraBruta = analogRead(PINO_MQ131);
  float tensao = leituraBruta * (5.0 / 1023.0);

  Serial.print("Valor Bruto (A0): ");
  Serial.print(leituraBruta);
  Serial.print("  |  Tensao: ");
  Serial.print(tensao, 2);
  Serial.println(" V");

  delay(1000);
}

//São utilizados jumpers macho-fêmea
//Relação dos pinos
// MQ-131     --------   Arduino UNO
//  VCC       ------- 5V (Arduino Uno)  fio Vermelho
//  GND       -------  GND (Arduino Uno) fio  Preto
//  A0        ------   A0 (Arduino Uno)  fio  Azul ou Verde
//  D0         -----   Sem conexão