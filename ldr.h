
#ifndef ESTRATOLAB_LDR_H
#define ESTRATOLAB_LDR_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"

#ifdef __cplusplus
extern "C" {
#endif

// GPIO5 corresponde ao ADC1_CH4
#define LDR_GPIO_PADRAO       5
#define LDR_CANAL_PADRAO      ADC_CHANNEL_4
#define LDR_NUM_AMOSTRAS      16

// Controle do LDR
typedef struct {
    adc_oneshot_unit_handle_t adc;
    adc_channel_t canal;
    bool iniciado;
} ldr_t;

// Dados coletados
typedef struct {
    uint16_t bruto;

    // Indice relativo, nao calibrado em lux
    float indice_relativo_pct;

    // Indica leitura proxima da saturacao
    bool proximo_limite_adc;
} ldr_amostra_t;

// Inicializa o LDR usando ADC ja configurado
esp_err_t ldr_iniciar(
    ldr_t *ldr,
    adc_oneshot_unit_handle_t adc,
    adc_channel_t canal
);

// Faz a leitura da luminosidade
esp_err_t ldr_ler(
    ldr_t *ldr,
    ldr_amostra_t *amostra
);

// Encerra o uso do LDR
void ldr_encerrar(ldr_t *ldr);

#ifdef __cplusplus
}
#endif

#endif // ESTRATOLAB_LDR_H
