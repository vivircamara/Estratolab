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

## Como Executar no Wokwi

1. Copie o código contido em `src/main.cpp` para o editor `sketch.ino` no Wokwi.
2. Copie o conteúdo de `wokwi/diagram.json` para a aba de diagrama visual.
3. Adicione as bibliotecas listadas em `wokwi/libraries.txt`.
4. Inicie a simulação e abra o **Serial Monitor** (115200 baud).

##  Compilação para a Placa Física (ESP32-S3)

Na **Arduino IDE**:
1. Placa: `ESP32S3 Dev Module`
2. Configuração crítica: **`USB CDC On Boot: Enabled`**
3. Atualize as definições de pinos no código (`I2C_SDA=8`, `I2C_SCL=9`, etc.).
