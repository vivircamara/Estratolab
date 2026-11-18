const int PINO_CO  = A1;
const int PINO_NO2 = A2;
const int PINO_NH3 = A3;

void setup() {
  Serial.begin(115200);
  while (!Serial);
}

void loop() {
  int leituraCO  = analogRead(PINO_CO);
  int leituraNO2 = analogRead(PINO_NO2);
  int leituraNH3 = analogRead(PINO_NH3);

  float tensaoCO  = leituraCO  * (5.0 / 1023.0);
  float tensaoNO2 = leituraNO2 * (5.0 / 1023.0);
  float tensaoNH3 = leituraNH3 * (5.0 / 1023.0);

  Serial.print("CO (A1): ");
  Serial.print(leituraCO);
  Serial.print(" (");
  Serial.print(tensaoCO, 2);
  Serial.print(" V)  |  ");

  Serial.print("NO2 (A2): ");
  Serial.print(leituraNO2);
  Serial.print(" (");
  Serial.print(tensaoNO2, 2);
  Serial.print(" V)  |  ");

  Serial.print("NH3 (A3): ");
  Serial.print(leituraNH3);
  Serial.print(" (");
  Serial.print(tensaoNH3, 2);
  Serial.println(" V)");

  delay(1000);
}

//São utilizados jumpers macho-fêmea
//Relação dos pinos
// MiCS-6814  --------   Arduino UNO
// - +5V       -------  5V (Arduino Uno) fio Vermelho
// - GND       -------  GND (Arduino Uno) fio Preto
// - CO        -------   A1 (Arduino Uno) fio Azul
// - NO2       -------   A2 (Arduino Uno) fio Verde
// - NH3       -------   A3 (Arduino Uno) fio Amarelo