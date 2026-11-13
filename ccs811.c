
#include "ccs811.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Registradores do CCS811
#define REG_STATUS       0x00
#define REG_MEAS_MODE    0x01
#define REG_ALG_RESULT   0x02
#define REG_ENV_DATA     0x05
#define REG_HW_ID        0x20
#define REG_ERROR_ID     0xE0
#define CMD_APP_START    0xF4

// Configuracoes do sensor
#define HW_ID_CCS811     0x81
#define STATUS_ERROR     0x01
#define STATUS_READY     0x08
#define STATUS_APP_VALID 0x10
#define STATUS_FW_MODE   0x80
#define MODO_1_SEGUNDO   0x10
#define TEMPO_I2C_MS     1000

static const char *TAG = "CCS811";

// Leitura de registradores via I2C
static esp_err_t ler_reg(
    ccs811_t *s,
    uint8_t reg,
    uint8_t *dest,
    size_t tam
) {
    if (!s || !s->dispositivo || !dest || tam == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_transmit_receive(
        s->dispositivo,
        &reg,
        1,
        dest,
        tam,
        TEMPO_I2C_MS
    );
}

// Escrita de dados via I2C
static esp_err_t escrever(
    ccs811_t *s,
    const uint8_t *dados,
    size_t tam
) {
    if (!s || !s->dispositivo || !dados || tam == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_transmit(
        s->dispositivo,
        dados,
        tam,
        TEMPO_I2C_MS
    );
}

// Encerra a comunicacao com o sensor
void ccs811_encerrar(ccs811_t *sensor)
{
    if (!sensor) {
        return;
    }

    if (sensor->dispositivo) {
        i2c_master_bus_rm_device(sensor->dispositivo);
    }

    sensor->dispositivo = NULL;
    sensor->inicio_medicoes_ms = 0;
}

// Inicializacao do CCS811
esp_err_t ccs811_iniciar(
    ccs811_t *sensor,
    i2c_master_bus_handle_t barramento,
    uint8_t endereco
) {
    if (!sensor || !barramento ||
        (endereco != 0x5A && endereco != 0x5B)) {
        return ESP_ERR_INVALID_ARG;
    }

    // O sensor deve estar zerado antes da inicializacao
    if (sensor->dispositivo) {
        return ESP_ERR_INVALID_STATE;
    }

    memset(sensor, 0, sizeof(*sensor));

    // Configuracao do dispositivo I2C
    i2c_device_config_t conf = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = endereco,
        .scl_speed_hz = 100000,
    };

    esp_err_t err = i2c_master_bus_add_device(
        barramento,
        &conf,
        &sensor->dispositivo
    );

    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(150));

    // Verifica a identificacao do sensor
    uint8_t id = 0;

    err = ler_reg(sensor, REG_HW_ID, &id, 1);

    if (err != ESP_OK) {
        goto erro;
    }

    if (id != HW_ID_CCS811) {
        ESP_LOGE(
            TAG,
            "HW_ID inesperado: 0x%02X",
            id
        );

        err = ESP_ERR_NOT_SUPPORTED;
        goto erro;
    }

    // Verifica o estado do sensor
    uint8_t status = 0;

    err = ler_reg(sensor, REG_STATUS, &status, 1);

    if (err != ESP_OK) {
        goto erro;
    }

    if (status & STATUS_ERROR) {
        uint8_t codigo = 0;

        (void)ler_reg(
            sensor,
            REG_ERROR_ID,
            &codigo,
            1
        );

        ESP_LOGE(
            TAG,
            "STATUS com erro: 0x%02X",
            codigo
        );

        err = ESP_FAIL;
        goto erro;
    }

    // Inicializa o firmware de aplicacao
    if (!(status & STATUS_FW_MODE)) {

        if (!(status & STATUS_APP_VALID)) {
            ESP_LOGE(
                TAG,
                "CCS811 sem firmware valido"
            );

            err = ESP_ERR_INVALID_STATE;
            goto erro;
        }

        const uint8_t comando = CMD_APP_START;

        err = escrever(sensor, &comando, 1);

        if (err != ESP_OK) {
            goto erro;
        }

        vTaskDelay(pdMS_TO_TICKS(150));

        err = ler_reg(
            sensor,
            REG_STATUS,
            &status,
            1
        );

        if (err != ESP_OK) {
            goto erro;
        }

        if (!(status & STATUS_FW_MODE) ||
            (status & STATUS_ERROR)) {

            err = ESP_ERR_INVALID_STATE;
            goto erro;
        }
    }

    // Configura uma medicao a cada segundo
    const uint8_t modo[] = {
        REG_MEAS_MODE,
        MODO_1_SEGUNDO
    };

    err = escrever(
        sensor,
        modo,
        sizeof(modo)
    );

    if (err != ESP_OK) {
        goto erro;
    }

    // Registra o instante de inicio das medicoes
    sensor->inicio_medicoes_ms =
        (uint64_t)esp_timer_get_time() / 1000ULL;

    ESP_LOGI(
        TAG,
        "CCS811 inicializado no endereco 0x%02X",
        endereco
    );

    return ESP_OK;

erro:
    ccs811_encerrar(sensor);
    return err;
}

// Leitura de eCO2 e TVOC
esp_err_t ccs811_ler(
    ccs811_t *sensor,
    ccs811_amostra_t *amostra,
    bool *nova
) {
    if (!sensor ||
        !sensor->dispositivo ||
        !amostra ||
        !nova) {

        return ESP_ERR_INVALID_ARG;
    }

    *nova = false;

    // Consulta o estado do sensor
    uint8_t status = 0;

    esp_err_t err = ler_reg(
        sensor,
        REG_STATUS,
        &status,
        1
    );

    if (err != ESP_OK) {
        return err;
    }

    // Verifica erros internos
    if (status & STATUS_ERROR) {

        uint8_t codigo = 0;

        (void)ler_reg(
            sensor,
            REG_ERROR_ID,
            &codigo,
            1
        );

        ESP_LOGE(
            TAG,
            "Erro interno do CCS811: 0x%02X",
            codigo
        );

        return ESP_FAIL;
    }

    // Confere se existe nova medicao
    if (!(status & STATUS_READY)) {
        return ESP_OK;
    }

    // Recebe os dados do sensor
    uint8_t dados[8] = {0};

    err = ler_reg(
        sensor,
        REG_ALG_RESULT,
        dados,
        sizeof(dados)
    );

    if (err != ESP_OK) {
        return err;
    }

    // Verifica erro retornado com a medicao
    if (dados[4] & STATUS_ERROR) {

        ESP_LOGE(
            TAG,
            "Erro na medicao: 0x%02X",
            dados[5]
        );

        return ESP_FAIL;
    }

    // Marca o horario da medicao
    uint64_t agora =
        (uint64_t)esp_timer_get_time() / 1000ULL;

    // Converte os bytes para valores inteiros
    amostra->eco2_ppm =
        (uint16_t)((dados[0] << 8) | dados[1]);

    amostra->tvoc_ppb =
        (uint16_t)((dados[2] << 8) | dados[3]);

    amostra->tempo_ms = agora;

    // Identifica o periodo inicial de aquecimento
    amostra->em_aquecimento =
        (agora - sensor->inicio_medicoes_ms)
        < CCS811_TEMPO_AQUECIMENTO_MS;

    *nova = true;

    return ESP_OK;
}

// Compensacao opcional com temperatura e umidade
esp_err_t ccs811_compensar_ambiente(
    ccs811_t *sensor,
    float temperatura_c,
    float umidade_relativa_pct
) {
    if (!sensor ||
        !sensor->dispositivo ||
        !isfinite(temperatura_c) ||
        !isfinite(umidade_relativa_pct) ||
        temperatura_c < -25.0f ||
        temperatura_c > 85.0f ||
        umidade_relativa_pct < 0.0f ||
        umidade_relativa_pct > 100.0f) {

        return ESP_ERR_INVALID_ARG;
    }

    // Converte umidade e temperatura
    // para o formato exigido pelo CCS811
    const uint16_t rh =
        (uint16_t)(
            umidade_relativa_pct * 512.0f + 0.5f
        );

    const uint16_t temp =
        (uint16_t)(
            (temperatura_c + 25.0f) * 512.0f + 0.5f
        );

    // Prepara os dados para envio
    uint8_t pacote[5] = {
        REG_ENV_DATA,
        (uint8_t)(rh >> 8),
        (uint8_t)rh,
        (uint8_t)(temp >> 8),
        (uint8_t)temp
    };

    // Envia a compensacao para o CCS811
    return escrever(
        sensor,
        pacote,
        sizeof(pacote)
    );
}
