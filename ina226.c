
#include "ina226.h"

#include <math.h>
#include <stddef.h>

// Registradores do INA226
#define INA226_REG_CONFIG   0x00
#define INA226_REG_SHUNT    0x01
#define INA226_REG_BUS      0x02
#define INA226_REG_FAB      0xFE
#define INA226_REG_CHIP     0xFF

// Identificacao do componente
#define INA226_ID_FAB       0x5449
#define INA226_ID_CHIP      0x2260

// Configuracoes de comunicacao
#define INA226_CONF_PADRAO  0x4127
#define INA226_I2C_HZ       100000
#define INA226_TIMEOUT_MS   1000

// Le um registrador de 16 bits
static esp_err_t ina226_ler_registro(
    ina226_t *monitor,
    uint8_t registrador,
    uint16_t *valor
) {
    if (!monitor || !monitor->dispositivo || !valor) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t bytes[2] = {0};

    esp_err_t erro = i2c_master_transmit_receive(
        monitor->dispositivo,
        &registrador,
        1,
        bytes,
        sizeof(bytes),
        INA226_TIMEOUT_MS
    );

    if (erro == ESP_OK) {
        *valor = (uint16_t)(
            ((uint16_t)bytes[0] << 8) | bytes[1]
        );
    }

    return erro;
}

// Escreve um registrador de 16 bits
static esp_err_t ina226_escrever_registro(
    ina226_t *monitor,
    uint8_t registrador,
    uint16_t valor
) {
    if (!monitor || !monitor->dispositivo) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t bytes[3] = {
        registrador,
        (uint8_t)(valor >> 8),
        (uint8_t)(valor & 0xFF)
    };

    return i2c_master_transmit(
        monitor->dispositivo,
        bytes,
        sizeof(bytes),
        INA226_TIMEOUT_MS
    );
}

// Encerra a comunicacao com o INA226
void ina226_encerrar(ina226_t *monitor) {
    if (!monitor) {
        return;
    }

    if (monitor->dispositivo) {
        (void)i2c_master_bus_rm_device(
            monitor->dispositivo
        );

        monitor->dispositivo = NULL;
    }

    monitor->resistor_shunt_ohms = 0.0f;
}

// Inicializa o monitor de energia
esp_err_t ina226_iniciar(
    ina226_t *monitor,
    i2c_master_bus_handle_t barramento,
    uint8_t endereco,
    float resistor_shunt_ohms
) {
    if (!monitor ||
        !barramento ||
        monitor->dispositivo ||
        endereco < 0x03 ||
        endereco > 0x77 ||
        !isfinite(resistor_shunt_ohms) ||
        resistor_shunt_ohms <= 0.0f) {

        return ESP_ERR_INVALID_ARG;
    }

    // Configura o dispositivo I2C
    i2c_device_config_t conf = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = endereco,
        .scl_speed_hz = INA226_I2C_HZ
    };

    esp_err_t erro = i2c_master_bus_add_device(
        barramento,
        &conf,
        &monitor->dispositivo
    );

    if (erro != ESP_OK) {
        return erro;
    }

    // Verifica a identificacao do componente
    uint16_t fabricante = 0;
    uint16_t chip = 0;

    erro = ina226_ler_registro(
        monitor,
        INA226_REG_FAB,
        &fabricante
    );

    if (erro != ESP_OK) {
        goto falha;
    }

    erro = ina226_ler_registro(
        monitor,
        INA226_REG_CHIP,
        &chip
    );

    if (erro != ESP_OK) {
        goto falha;
    }

    // Confere fabricante e modelo
    if (fabricante != INA226_ID_FAB ||
        (chip & 0xFFF0U) != INA226_ID_CHIP) {

        erro = ESP_ERR_NOT_SUPPORTED;
        goto falha;
    }

    // Ativa leituras continuas de tensao
    // do barramento e do resistor shunt
    erro = ina226_escrever_registro(
        monitor,
        INA226_REG_CONFIG,
        INA226_CONF_PADRAO
    );

    if (erro != ESP_OK) {
        goto falha;
    }

    monitor->resistor_shunt_ohms =
        resistor_shunt_ohms;

    return ESP_OK;

falha:
    ina226_encerrar(monitor);
    return erro;
}

// Realiza a leitura do INA226
esp_err_t ina226_ler(
    ina226_t *monitor,
    ina226_amostra_t *amostra
) {
    if (!monitor ||
        !monitor->dispositivo ||
        !amostra ||
        monitor->resistor_shunt_ohms <= 0.0f) {

        return ESP_ERR_INVALID_ARG;
    }

    uint16_t bruto_shunt = 0;
    uint16_t bruto_bus = 0;

    // Le a tensao no resistor shunt
    esp_err_t erro = ina226_ler_registro(
        monitor,
        INA226_REG_SHUNT,
        &bruto_shunt
    );

    if (erro != ESP_OK) {
        return erro;
    }

    // Le a tensao do barramento
    erro = ina226_ler_registro(
        monitor,
        INA226_REG_BUS,
        &bruto_bus
    );

    if (erro != ESP_OK) {
        return erro;
    }

    // Converte a leitura do shunt
    // para um valor com sinal
    int32_t shunt_assinado =
        (bruto_shunt & 0x8000U)
        ? ((int32_t)bruto_shunt - 65536)
        : (int32_t)bruto_shunt;

    // Cada unidade do shunt representa 2,5 uV
    float tensao_shunt_v =
        (float)shunt_assinado * 0.0000025f;

    // Cada unidade do barramento representa 1,25 mV
    float tensao_v =
        (float)(bruto_bus & 0x7FFFU) * 0.00125f;

    // Calcula corrente pela Lei de Ohm
    float corrente_a =
        tensao_shunt_v /
        monitor->resistor_shunt_ohms;

    // Calcula a potencia eletrica
    ina226_amostra_t resultado = {
        .tensao_v = tensao_v,
        .corrente_a = corrente_a,
        .potencia_w = tensao_v * corrente_a,
        .tensao_shunt_mv = tensao_shunt_v * 1000.0f,

        // Identifica leitura no limite de escala
        .saturado =
            (bruto_shunt == 0x7FFFU ||
             bruto_shunt == 0x8000U ||
             (bruto_bus & 0x7FFFU) == 0x7FFFU)
    };

    *amostra = resultado;

    return ESP_OK;
}
