
#ifndef ESTRATOLAB_INA226_H
#define ESTRATOLAB_INA226_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Endereco padrao I2C
#define INA226_ENDERECO_PADRAO 0x40

// Estrutura de controle do monitor
typedef struct {
    i2c_master_dev_handle_t dispositivo;
    float resistor_shunt_ohms;
} ina226_t;

// Estrutura dos dados coletados
typedef struct {
    float tensao_v;
    float corrente_a;
    float potencia_w;
    float tensao_shunt_mv;
    bool saturado;
} ina226_amostra_t;

// Inicializa o INA226
esp_err_t ina226_iniciar(
    ina226_t *monitor,
    i2c_master_bus_handle_t barramento,
    uint8_t endereco,
    float resistor_shunt_ohms
);

// Le tensao, corrente e potencia
esp_err_t ina226_ler(
    ina226_t *monitor,
    ina226_amostra_t *amostra
);

// Encerra a comunicacao
void ina226_encerrar(
    ina226_t *monitor
);

#ifdef __cplusplus
}
#endif

#endif // ESTRATOLAB_INA226_H
