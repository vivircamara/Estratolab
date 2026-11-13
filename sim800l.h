
#ifndef ESTRATOLAB_SIM800L_H
#define ESTRATOLAB_SIM800L_H

#include <stdbool.h>
#include <stddef.h>
#include "driver/uart.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Um modem por UART.
// O programa principal deve reservar a UART escolhida.
typedef struct {
    uart_port_t uart;
    bool iniciado;
} sim800l_t;

// Informacoes sobre o estado do SIM800L
typedef struct {
    bool sim_pronto;  // Cartao SIM detectado e liberado

    int rssi;         // Sinal: 0 a 31
                      // 99 = desconhecido

    int ber;          // Taxa de erro: 0 a 7
                      // 99 = desconhecido

    int registro;     // 1 = rede local
                      // 5 = roaming
                      // -1 = desconhecido

    int tensao_mv;    // Tensao da fonte em mV
                      // -1 = desconhecida

} sim800l_estado_t;

// Inicializa o modulo SIM800L
// Os pinos TX e RX pertencem ao ESP32-S3
esp_err_t sim800l_iniciar(
    sim800l_t *modem,
    uart_port_t uart,
    int gpio_tx,
    int gpio_rx,
    int baud_rate
);

// Envia comandos AT e recebe respostas
esp_err_t sim800l_comando(
    sim800l_t *modem,
    const char *comando,
    char *resposta,
    size_t capacidade,
    unsigned tempo_ms
);

// Consulta as informacoes do modulo
esp_err_t sim800l_consultar(
    sim800l_t *modem,
    sim800l_estado_t *estado
);

// Envia mensagem SMS, quando solicitado
esp_err_t sim800l_enviar_sms(
    sim800l_t *modem,
    const char *numero,
    const char *mensagem
);

// Encerra a comunicacao com o modulo
void sim800l_encerrar(
    sim800l_t *modem
);

#ifdef __cplusplus
}
#endif

#endif // ESTRATOLAB_SIM800L_H
