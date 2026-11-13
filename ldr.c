
#include "ldr.h"

#include <stddef.h>

#define LDR_ADC_MAXIMO 4095U
#define LDR_MARGEM_SATURACAO 10U

// Inicializa o canal analogico do LDR
esp_err_t ldr_iniciar(
    ldr_t *ldr,
    adc_oneshot_unit_handle_t adc,
    adc_channel_t canal
) {
    if (ldr == NULL || adc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (ldr->iniciado) {
        return ESP_ERR_INVALID_STATE;
    }

    // Configuracao do conversor analogico
    adc_oneshot_chan_cfg_t configuracao = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12
    };

    esp_err_t erro = adc_oneshot_config_channel(
        adc,
        canal,
        &configuracao
    );

    if (erro != ESP_OK) {
        return erro;
    }

    ldr->adc = adc;
    ldr->canal = canal;
    ldr->iniciado = true;

    return ESP_OK;
}

// Realiza leitura da luminosidade
esp_err_t ldr_ler(
    ldr_t *ldr,
    ldr_amostra_t *amostra
) {
    if (ldr == NULL ||
        !ldr->iniciado ||
        ldr->adc == NULL ||
        amostra == NULL) {

        return ESP_ERR_INVALID_ARG;
    }

    uint32_t soma = 0;

    // Faz 16 leituras para reduzir ruido
    for (unsigned i = 0;
         i < LDR_NUM_AMOSTRAS;
         i++) {

        int bruto = 0;

        esp_err_t erro = adc_oneshot_read(
            ldr->adc,
            ldr->canal,
            &bruto
        );

        if (erro != ESP_OK) {
            return erro;
        }

        // Verifica se a leitura esta na escala
        if (bruto < 0 ||
            bruto > (int)LDR_ADC_MAXIMO) {

            return ESP_FAIL;
        }

        soma += (uint32_t)bruto;
    }

    // Calcula a media das leituras
    uint16_t media =
        (uint16_t)(soma / LDR_NUM_AMOSTRAS);

    // Organiza os dados coletados
    ldr_amostra_t resultado = {
        .bruto = media,

        .indice_relativo_pct =
            ((float)media * 100.0f) /
            (float)LDR_ADC_MAXIMO,

        .proximo_limite_adc =
            (media <= LDR_MARGEM_SATURACAO ||
             media >= LDR_ADC_MAXIMO -
                      LDR_MARGEM_SATURACAO)
    };

    *amostra = resultado;

    return ESP_OK;
}

// Encerra o uso do LDR
void ldr_encerrar(ldr_t *ldr) {
    if (ldr == NULL) {
        return;
    }

    // Nao desliga o ADC compartilhado
    ldr->adc = NULL;
    ldr->canal = ADC_CHANNEL_0;
    ldr->iniciado = false;
}
