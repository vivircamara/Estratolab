# Estratolab - Payload para Balão Estratosférico - ESP32-S3 / ESP32

Projeto de telemetria e aquisição de dados ambientais para payload estratosférico utilizando ESP32.

## Sensores Utilizados

- **BMP280**: Pressão atmosférica, temperatura e cálculo de altitude.
- **DHT22**: Temperatura e umidade relativa do ar.
- **MPU-6050**: Acelerômetro e giroscópio (orientação e atitude da sonda).
- **ML8511**: Sensor de radiação Ultravioleta (UV).
- **MQ-131**: Sensor de concentração de Ozônio ($O_3$).
- **BPW34**: Fotodiodo para radiação solar/luminosidade.

## Mapeamento de Pinos (ESP32-S3 Real vs. Wokwi)

| Periférico / Sensor | Sinal | Pino ESP32-S3 (Placa Real) | Pino ESP32 (Simulação Wokwi) |
|---------------------|-------|----------------------------|------------------------------|
| **DHT22**           | Data  | GPIO 4                     | GPIO 4                       |
| **Barramento I2C**  | SDA   | GPIO 8                     | GPIO 21                      |
| **Barramento I2C**  | SCL   | GPIO 9                     | GPIO 22                      |
| **ML8511 (UV)**     | OUT   | GPIO 1 (ADC1_CH0)          | GPIO 36 (VP)                 |
| **MQ-131 (Ozônio)** | OUT   | GPIO 2 (ADC1_CH1)          | GPIO 39 (VN)                 |
| **BPW34 (Rad)**     | OUT   | GPIO 3 (ADC1_CH2)          | GPIO 34                      |


