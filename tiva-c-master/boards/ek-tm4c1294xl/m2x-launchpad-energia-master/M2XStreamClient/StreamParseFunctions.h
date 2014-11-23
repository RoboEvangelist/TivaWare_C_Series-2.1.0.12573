#ifndef StreamParseFunctions_h
#define StreamParseFunctions_h

// Data structures and functions used to parse stream values

#define STREAM_BUF_LEN 20

typedef struct {
  uint8_t state;
  char at_str[STREAM_BUF_LEN + 1];
  char value_str[STREAM_BUF_LEN + 1];
  int index;

  stream_value_read_callback callback;
  void* context;
} stream_parsing_context_state;

#define WAITING_AT 0x1
#define GOT_AT 0x2
#define WAITING_VALUE 0x4
#define GOT_VALUE 0x8

#define GOT_STREAM (GOT_AT | GOT_VALUE)
#define TEST_GOT_STREAM(state_) (((state_) & GOT_STREAM) == GOT_STREAM)

#define TEST_IS_AT(state_) (((state_) & (WAITING_AT | GOT_AT)) == WAITING_AT)
#define TEST_IS_VALUE(state_) (((state_) & (WAITING_VALUE | GOT_VALUE)) == \
                               WAITING_VALUE)

static void on_stream_key_found(jsonlite_callback_context* context,
                                jsonlite_token* token)
{
  stream_parsing_context_state* state =
      (stream_parsing_context_state*) context->client_state;
  if (strncmp((const char*) token->start, "at", 2) == 0) {
    state->state |= WAITING_AT;
  } else if ((strncmp((const char*) token->start, "value", 5) == 0) &&
             (token->start[5] != 's')) { // get rid of "values"
    state->state |= WAITING_VALUE;
  }
}

static void on_stream_string_found(jsonlite_callback_context* context,
                                   jsonlite_token* token)
{
  stream_parsing_context_state* state =
      (stream_parsing_context_state*) context->client_state;

  if (TEST_IS_AT(state->state)) {
    strncpy(state->at_str, (const char*) token->start,
            MIN(token->end - token->start, STREAM_BUF_LEN));
    state->at_str[MIN(token->end - token->start, STREAM_BUF_LEN)] = '\0';
    state->state |= GOT_AT;
  } else if (TEST_IS_VALUE(state->state)) {
    strncpy(state->value_str, (const char*) token->start,
            MIN(token->end - token->start, STREAM_BUF_LEN));
    state->value_str[MIN(token->end - token->start, STREAM_BUF_LEN)] = '\0';
    state->state |= GOT_VALUE;
  }

  if (TEST_GOT_STREAM(state->state)) {
    state->callback(state->at_str, state->value_str,
                    state->index++, state->context);
    state->state = 0;
  }
}

#endif  /* StreamParseFunctions_h */
