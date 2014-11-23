#ifndef LocationParseFunctions_h
#define LocationParseFunctions_h

// MSP430 does not have atof. Include local version
#ifdef __MSP430MCU__
#include "atof.h"
#endif

// Data structures and functions used to parse locations

#define LOCATION_BUF_LEN 20

typedef struct {
  uint16_t state;
  char name_str[LOCATION_BUF_LEN + 1];
  double latitude;
  double longitude;
  double elevation;
  char timestamp_str[LOCATION_BUF_LEN + 1];
  int index;

  location_read_callback callback;
  void* context;
} location_parsing_context_state;

#define WAITING_NAME 0x1
#define WAITING_LATITUDE 0x2
#define WAITING_LONGITUDE 0x4
#define WAITING_ELEVATION 0x8
#define WAITING_TIMESTAMP 0x10

#define GOT_NAME 0x20
#define GOT_LATITUDE 0x40
#define GOT_LONGITUDE 0x80
#define GOT_ELEVATION 0x100
#define GOT_TIMESTAMP 0x200

#define GOT_LOCATION (GOT_NAME | GOT_LATITUDE | GOT_LONGITUDE | GOT_ELEVATION | GOT_TIMESTAMP)
#define TEST_GOT_LOCATION(state_) (((state_) & GOT_LOCATION) == GOT_LOCATION)

#define TEST_IS_NAME(state_) (((state_) & (WAITING_NAME | GOT_NAME)) == WAITING_NAME)
#define TEST_IS_LATITUDE(state_) (((state_) & (WAITING_LATITUDE | GOT_LATITUDE)) \
                                  == WAITING_LATITUDE)
#define TEST_IS_LONGITUDE(state_) (((state_) & (WAITING_LONGITUDE | GOT_LONGITUDE)) \
                                   == WAITING_LONGITUDE)
#define TEST_IS_ELEVATION(state_) (((state_) & (WAITING_ELEVATION | GOT_ELEVATION)) \
                                   == WAITING_ELEVATION)
#define TEST_IS_TIMESTAMP(state_) (((state_) & (WAITING_TIMESTAMP | GOT_TIMESTAMP)) \
                                   == WAITING_TIMESTAMP)

static void on_location_key_found(jsonlite_callback_context* context,
                                  jsonlite_token* token) {
  location_parsing_context_state* state =
      (location_parsing_context_state*) context->client_state;
  if (strncmp((const char*) token->start, "waypoints", 9) == 0) {
    // only parses those locations in waypoints, skip the outer one
    state->state = 0;
  } else if (strncmp((const char*) token->start, "name", 4) == 0) {
    state->state |= WAITING_NAME;
  } else if (strncmp((const char*) token->start, "latitude", 8) == 0) {
    state->state |= WAITING_LATITUDE;
  } else if (strncmp((const char*) token->start, "longitude", 9) == 0) {
    state->state |= WAITING_LONGITUDE;
  } else if (strncmp((const char*) token->start, "elevation", 9) == 0) {
    state->state |= WAITING_ELEVATION;
  } else if (strncmp((const char*) token->start, "timestamp", 9) == 0) {
    state->state |= WAITING_TIMESTAMP;
  }
}

static void on_location_string_found(jsonlite_callback_context* context,
                                     jsonlite_token* token) {
  location_parsing_context_state* state =
      (location_parsing_context_state*) context->client_state;

  if (TEST_IS_NAME(state->state)) {
    strncpy(state->name_str, (const char*) token->start,
            MIN(token->end - token->start, LOCATION_BUF_LEN));
    state->name_str[MIN(token->end - token->start, LOCATION_BUF_LEN)] = '\0';
    state->state |= GOT_NAME;
  } else if (TEST_IS_LATITUDE(state->state)) {
    state->latitude = atof((const char*) token->start);
    state->state |= GOT_LATITUDE;
  } else if (TEST_IS_LONGITUDE(state->state)) {
    state->longitude = atof((const char*) token->start);
    state->state |= GOT_LONGITUDE;
  } else if (TEST_IS_ELEVATION(state->state)) {
    state->elevation = atof((const char*) token->start);
    state->state |= GOT_ELEVATION;
  } else if (TEST_IS_TIMESTAMP(state->state)) {
    strncpy(state->timestamp_str, (const char*) token->start,
            MIN(token->end - token->start, LOCATION_BUF_LEN));
    state->timestamp_str[MIN(token->end - token->start, LOCATION_BUF_LEN)] = '\0';
    state->state |= GOT_TIMESTAMP;
  }

  if (TEST_GOT_LOCATION(state->state)) {
    state->callback(state->name_str, state->latitude, state->longitude,
                    state->elevation, state->timestamp_str, state->index++,
                    state->context);
    state->state = 0;
  }
}

#endif  /* LocationParseFunctions_h */
