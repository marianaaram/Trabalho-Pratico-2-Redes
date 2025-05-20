
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define MAX_TEXT_LEN 140 // Tamanho máximo do texto da mensagem

// Tipos de mensagem
#define MSG_OI     0
#define MSG_TCHAU  1
#define MSG_MSG    2

// Estrutura da mensagem
typedef struct {
    uint16_t type;
    uint16_t orig_uid;
    uint16_t dest_uid;
    uint16_t text_len;
    uint8_t  text[MAX_TEXT_LEN + 1]; // +1 para '\0'
} msg_t;

#endif
