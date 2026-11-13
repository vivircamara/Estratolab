
#ifndef ESTRATOLAB_CCS811_H
#define ESTRATOLAB_CCS811_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

// Compatibilidade entre C e C++
#ifdef __cplusplus
extern "C" {
#endif

// Configuracoes do CCS811
#define CCS811_ENDERECO_PADRAO 0x5A

// Tempo inicial de aquecimento: 20 minutos
#define CCS811_TEMPO_AQUECIMENTO_MS \
    (20ULL * 60ULL * 1000ULL)

// Estrutura de controle do sensor
typedef struct {
    i2c_master_dev_handle_t dispositivo;
    uint64_t inicio_medicoes_ms;
} ccs811_t;

// Estrutura dos dados coletados
typedef struct {
    // CO2 equivalente estimado (ppm)
    uint16_t eco2_ppm;

    // Compostos organicos volateis (ppb)
    uint16_t tvoc_ppb;

    // Tempo desde o inicio do ESP32-S3
    uint64_t tempo_ms;

    // Indica periodo inicial de aquecimento
    bool em_aquecimento;

} ccs811_amostra_t;

// Inicializa o sensor CCS811
// Utiliza um barramento I2C ja configurado
esp_err_t ccs811_iniciar(
    ccs811_t *sensor,
    i2c_master_bus_handle_t barramento,
    uint8_t endereco
);

// Realiza a leitura do sensor
// Retorna eCO2 e TVOC
esp_err_t ccs811_ler(
    ccs811_t *sensor,
    ccs811_amostra_t *amostra,
    bool *nova
);

// Compensacao opcional com temperatura
// e umidade obtidas de outro sensor
esp_err_t ccs811_compensar_ambiente(
    ccs811_t *sensor,
    float temperatura_c,
    float umidade_relativa_pct
);

// Encerra a comunicacao com o CCS811
void ccs811_encerrar(
    ccs811_t *sensor
);

// Finaliza compatibilidade com C++
#ifdef __cplusplus
}
#endif

#endif // ESTRATOLAB_CCS811_H
