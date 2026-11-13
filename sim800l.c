
#include "sim800l.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Configuracoes dos buffers UART
#define SIM800L_BUFFER_RX 1024
#define SIM800L_BUFFER_TX 1024

// Aguarda uma resposta do SIM800L
static esp_err_t esperar_texto(
    sim800l_t *modem,
    char *resposta,
    size_t capacidade,
    const char *esperado,
    unsigned tempo_ms
) {
    if (!modem || !modem->iniciado ||
        !resposta || capacidade < 2 ||
        !esperado || tempo_ms == 0) {

        return ESP_ERR_INVALID_ARG;
    }

    resposta[0] = '\0';

    size_t usado = 0;

    TickType_t inicio = xTaskGetTickCount();
    TickType_t prazo = pdMS_TO_TICKS(tempo_ms);

    if (prazo == 0) {
        prazo = 1;
    }

    while (
        (TickType_t)(xTaskGetTickCount() - inicio)
        < prazo
    ) {
        uint8_t bytes[48];

        int recebidos = uart_read_bytes(
            modem->uart,
            bytes,
            sizeof(bytes),
            pdMS_TO_TICKS(100)
        );

        if (recebidos < 0) {
            return ESP_FAIL;
        }

        for (int i = 0; i < recebidos; i++) {

            if (usado + 1 >= capacidade) {
                return ESP_ERR_INVALID_SIZE;
            }

            resposta[usado++] = (char)bytes[i];
        }

        resposta[usado] = '\0';

        // Detecta erros informados pelo modem
        if (
            strstr(resposta, "\r\nERROR\r\n") ||
            strstr(resposta, "+CME ERROR:") ||
            strstr(resposta, "+CMS ERROR:")
        ) {
            return ESP_FAIL;
        }

        if (strstr(resposta, esperado)) {
            return ESP_OK;
        }
    }

    return ESP_ERR_TIMEOUT;
}

// Envia comandos AT ao SIM800L
esp_err_t sim800l_comando(
    sim800l_t *modem,
    const char *comando,
    char *resposta,
    size_t capacidade,
    unsigned tempo_ms
) {
    if (!modem ||
        !modem->iniciado ||
        !comando ||
        strncmp(comando, "AT", 2) != 0 ||
        strlen(comando) > 80 ||
        !resposta ||
        capacidade < 2) {

        return ESP_ERR_INVALID_ARG;
    }

    // Remove notificacoes antigas da UART
    esp_err_t erro = uart_flush_input(modem->uart);

    if (erro != ESP_OK) {
        return erro;
    }

    int tamanho = (int)strlen(comando);

    if (
        uart_write_bytes(
            modem->uart,
            comando,
            tamanho
        ) != tamanho ||

        uart_write_bytes(
            modem->uart,
            "\r",
            1
        ) != 1
    ) {
        return ESP_FAIL;
    }

    return esperar_texto(
        modem,
        resposta,
        capacidade,
        "\r\nOK\r\n",
        tempo_ms
    );
}

// Inicializa a comunicacao com o SIM800L
esp_err_t sim800l_iniciar(
    sim800l_t *modem,
    uart_port_t uart,
    int gpio_tx,
    int gpio_rx,
    int baud_rate
) {
    if (
        !modem ||
        modem->iniciado ||
        uart < UART_NUM_0 ||
        uart >= UART_NUM_MAX ||
        gpio_tx < 0 ||
        gpio_rx < 0 ||
        gpio_tx == gpio_rx ||
        baud_rate <= 0
    ) {
        return ESP_ERR_INVALID_ARG;
    }

    // Configuracao da UART
    uart_config_t cfg = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t erro = uart_param_config(
        uart,
        &cfg
    );

    if (erro != ESP_OK) {
        return erro;
    }

    // Define os pinos TX e RX
    erro = uart_set_pin(
        uart,
        gpio_tx,
        gpio_rx,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    );

    if (erro != ESP_OK) {
        return erro;
    }

    // Instala o driver UART
    erro = uart_driver_install(
        uart,
        SIM800L_BUFFER_RX,
        SIM800L_BUFFER_TX,
        0,
        NULL,
        0
    );

    if (erro != ESP_OK) {
        return erro;
    }

    modem->uart = uart;
    modem->iniciado = true;

    // Verifica se o modem esta respondendo
    char resposta[192];

    for (int tentativa = 0; tentativa < 5; tentativa++) {

        if (
            sim800l_comando(
                modem,
                "AT",
                resposta,
                sizeof(resposta),
                1500
            ) == ESP_OK
        ) {
            // Desliga o eco dos comandos
            (void)sim800l_comando(
                modem,
                "ATE0",
                resposta,
                sizeof(resposta),
                1500
            );

            return ESP_OK;
        }

        vTaskDelay(pdMS_TO_TICKS(700));
    }

    // Encerra caso o modem nao responda
    sim800l_encerrar(modem);

    return ESP_ERR_TIMEOUT;
}

// Consulta o estado do SIM800L
esp_err_t sim800l_consultar(
    sim800l_t *modem,
    sim800l_estado_t *estado
) {
    if (
        !modem ||
        !modem->iniciado ||
        !estado
    ) {
        return ESP_ERR_INVALID_ARG;
    }

    // Valores iniciais
    estado->sim_pronto = false;
    estado->rssi = 99;
    estado->ber = 99;
    estado->registro = -1;
    estado->tensao_mv = -1;

    char resposta[256];

    // Verifica se o chip SIM esta pronto
    esp_err_t erro = sim800l_comando(
        modem,
        "AT+CPIN?",
        resposta,
        sizeof(resposta),
        3000
    );

    if (erro != ESP_OK) {
        return erro;
    }

    estado->sim_pronto =
        strstr(resposta, "+CPIN: READY") != NULL;

    // Consulta a intensidade do sinal
    if (
        sim800l_comando(
            modem,
            "AT+CSQ",
            resposta,
            sizeof(resposta),
            3000
        ) == ESP_OK
    ) {
        const char *p = strstr(resposta, "+CSQ:");

        if (p) {
            int rssi, ber;

            if (
                sscanf(
                    p,
                    "+CSQ: %d,%d",
                    &rssi,
                    &ber
                ) == 2
            ) {
                estado->rssi = rssi;
                estado->ber = ber;
            }
        }
    }

    // Verifica o registro na rede GSM
    if (
        sim800l_comando(
            modem,
            "AT+CREG?",
            resposta,
            sizeof(resposta),
            3000
        ) == ESP_OK
    ) {
        const char *p = strstr(resposta, "+CREG:");

        if (p) {
            int modo, reg;

            if (
                sscanf(
                    p,
                    "+CREG: %d,%d",
                    &modo,
                    &reg
                ) == 2
            ) {
                estado->registro = reg;
            }
        }
    }

    // Consulta a tensao do SIM800L
    if (
        sim800l_comando(
            modem,
            "AT+CBC",
            resposta,
            sizeof(resposta),
            3000
        ) == ESP_OK
    ) {
        const char *p = strstr(resposta, "+CBC:");

        if (p) {
            int bcs, bcl, mv;

            if (
                sscanf(
                    p,
                    "+CBC: %d,%d,%d",
                    &bcs,
                    &bcl,
                    &mv
                ) == 3
            ) {
                estado->tensao_mv = mv;
            }
        }
    }

    return ESP_OK;
}

// Verifica se o numero de telefone e valido
static bool numero_valido(const char *numero)
{
    if (!numero) {
        return false;
    }

    size_t len = strlen(numero);

    if (len < 8 || len > 16) {
        return false;
    }

    // Permite '+' no inicio e apenas digitos
    for (
        size_t i = (numero[0] == '+') ? 1 : 0;
        i < len;
        i++
    ) {
        if (!isdigit((unsigned char)numero[i])) {
            return false;
        }
    }

    return true;
}

// Envia uma mensagem SMS
esp_err_t sim800l_enviar_sms(
    sim800l_t *modem,
    const char *numero,
    const char *mensagem
) {
    if (
        !modem ||
        !modem->iniciado ||
        !numero_valido(numero) ||
        !mensagem ||
        strlen(mensagem) == 0 ||
        strlen(mensagem) > 140
    ) {
        return ESP_ERR_INVALID_ARG;
    }

    char resposta[512];

    // Define o modo de SMS como texto
    esp_err_t erro = sim800l_comando(
        modem,
        "AT+CMGF=1",
        resposta,
        sizeof(resposta),
        3000
    );

    if (erro != ESP_OK) {
        return erro;
    }

    // Prepara o destinatario
    char cmd[48];

    int n = snprintf(
        cmd,
        sizeof(cmd),
        "AT+CMGS=\"%s\"\r",
        numero
    );

    if (
        n <= 0 ||
        (size_t)n >= sizeof(cmd)
    ) {
        return ESP_ERR_INVALID_SIZE;
    }

    erro = uart_flush_input(modem->uart);

    if (erro != ESP_OK) {
        return erro;
    }

    // Envia o numero ao modem
    if (
        uart_write_bytes(
            modem->uart,
            cmd,
            n
        ) != n
    ) {
        return ESP_FAIL;
    }

    // Aguarda o modem solicitar a mensagem
    erro = esperar_texto(
        modem,
        resposta,
        sizeof(resposta),
        ">",
        10000
    );

    if (erro != ESP_OK) {
        return erro;
    }

    // Envia o texto da mensagem
    int qtd = (int)strlen(mensagem);

    if (
        uart_write_bytes(
            modem->uart,
            mensagem,
            qtd
        ) != qtd
    ) {
        return ESP_FAIL;
    }

    // Ctrl+Z finaliza o envio da mensagem
    const char ctrl_z = 0x1A;

    if (
        uart_write_bytes(
            modem->uart,
            &ctrl_z,
            1
        ) != 1
    ) {
        return ESP_FAIL;
    }

    // Aguarda a confirmacao do modem
    return esperar_texto(
        modem,
        resposta,
        sizeof(resposta),
        "\r\nOK\r\n",
        60000
    );
}

// Encerra a comunicacao UART
void sim800l_encerrar(sim800l_t *modem)
{
    if (!modem || !modem->iniciado) {
        return;
    }

    // Remove o driver UART
    (void)uart_driver_delete(modem->uart);

    modem->iniciado = false;
}
