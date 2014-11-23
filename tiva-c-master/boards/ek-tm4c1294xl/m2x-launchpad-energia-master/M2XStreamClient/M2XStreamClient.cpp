#include "M2XStreamClient.h"

#include <jsonlite.h>

#include "StreamParseFunctions.h"
#include "LocationParseFunctions.h"

const char* M2XStreamClient::kDefaultM2XHost = "api-m2x.att.com";

int print_encoded_string(Print* print, const char* str);
int tolower(int ch);

#if defined(ARDUINO_PLATFORM) || defined(MBED_PLATFORM)
int tolower(int ch)
{
  // Arduino and mbed use ASCII table, so we can simplify the implementation
  if ((ch >= 'A') && (ch <= 'Z')) {
    return (ch + 32);
  }
  return ch;
}
#else
// For other platform, we use libc's tolower by default
#include <ctype.h>
#endif

M2XStreamClient::M2XStreamClient(Client* client,
                                 const char* key,
                                 int case_insensitive,
                                 const char* host,
                                 int port) : _client(client),
                                             _key(key),
                                             _case_insensitive(case_insensitive),
                                             _host(host),
                                             _port(port),
                                             _null_print() {
}

#define WRITE_QUERY_PART(client_, started_, name_, str_) { \
  if (str_) { \
    if (started_) { \
      (client_)->print("&"); \
    } else { \
      (client_)->print("?"); \
      started_ = true; \
    } \
    (client_)->print(name_ "="); \
    (client_)->print(str_); \
  } \
  }

int M2XStreamClient::fetchValues(const char* feedId, const char* streamName,
                                 stream_value_read_callback callback, void* context,
                                 const char* startTime, const char* endTime,
                                 const char* limit) {
  if (_client->connect(_host, _port)) {
    bool query_started = false;

    DBGLN("%s", "Connected to M2X server!");
    _client->print("GET /v1/feeds/");
    print_encoded_string(_client, feedId);
    _client->print("/streams/");
    print_encoded_string(_client, streamName);
    _client->print("/values");

    WRITE_QUERY_PART(_client, query_started, "start", startTime);
    WRITE_QUERY_PART(_client, query_started, "end", endTime);
    WRITE_QUERY_PART(_client, query_started, "limit", limit);

    _client->println(" HTTP/1.0");
    writeHttpHeader(-1);
  } else {
    DBGLN("%s", "ERROR: Cannot connect to M2X server!");
    return E_NOCONNECTION;
  }
  int status = readStatusCode(false);
  if (status == 200) {
    readStreamValue(callback, context);
  }

  close();
  return status;
}

int M2XStreamClient::readLocation(const char* feedId,
                                  location_read_callback callback,
                                  void* context) {
  if (_client->connect(_host, _port)) {
    DBGLN("%s", "Connected to M2X server!");
    _client->print("GET /v1/feeds/");
    print_encoded_string(_client, feedId);
    _client->println("/location HTTP/1.0");

    writeHttpHeader(-1);
  } else {
    DBGLN("%s", "ERROR: Cannot connect to M2X server!");
    return E_NOCONNECTION;
  }
  int status = readStatusCode(false);
  if (status == 200) {
    readLocation(callback, context);
  }

  close();
  return status;
}

// Encodes and prints string using Percent-encoding specified
// in RFC 1738, Section 2.2
int print_encoded_string(Print* print, const char* str) {
  int bytes = 0;
  for (int i = 0; str[i] != 0; i++) {
    if (((str[i] >= 'A') && (str[i] <= 'Z')) ||
        ((str[i] >= 'a') && (str[i] <= 'z')) ||
        ((str[i] >= '0') && (str[i] <= '9')) ||
        (str[i] == '-') || (str[i] == '_') ||
        (str[i] == '.') || (str[i] == '~')) {
      bytes += print->print(str[i]);
    } else {
      // Encode all other characters
      bytes += print->print('%');
      bytes += print->print(HEX(str[i] / 16));
      bytes += print->print(HEX(str[i] % 16));
    }
  }
  return bytes;
}

void M2XStreamClient::writePostHeader(const char* feedId,
                                      const char* streamName,
                                      int contentLength) {
  _client->print("PUT /v1/feeds/");
  print_encoded_string(_client, feedId);
  _client->print("/streams/");
  print_encoded_string(_client, streamName);
  _client->println(" HTTP/1.0");

  writeHttpHeader(contentLength);
}

void M2XStreamClient::writeHttpHeader(int contentLength) {
  _client->println(USER_AGENT);
  _client->print("X-M2X-KEY: ");
  _client->println(_key);

  _client->print("Host: ");
  print_encoded_string(_client, _host);
  if (_port != kDefaultM2XPort) {
    _client->print(":");
    // port is an integer, does not need encoding
    _client->print(_port);
  }
  _client->println();

  if (contentLength > 0) {
    _client->println("Content-Type: application/json");
    DBG("%s", "Content Length: ");
    DBGLN("%d", contentLength);

    _client->print("Content-Length: ");
    _client->println(contentLength);
  }
  _client->println();
}

int M2XStreamClient::waitForString(const char* str) {
  int currentIndex = 0;
  if (str[currentIndex] == '\0') return E_OK;

  while (true) {
    while (_client->available()) {
      char c = _client->read();
      DBG("%c", c);

      int cmp;
      if (_case_insensitive) {
        cmp = tolower(c) - tolower(str[currentIndex]);
      } else {
        cmp = c - str[currentIndex];
      }

      if ((str[currentIndex] == '*') || (cmp == 0)) {
        currentIndex++;
        if (str[currentIndex] == '\0') {
          return E_OK;
        }
      } else {
        // start from the beginning
        currentIndex = 0;
      }
    }

    if (!_client->connected()) {
      DBGLN("%s", "ERROR: The client is disconnected from the server!");

      close();
      return E_DISCONNECTED;
    }

    delay(1000);
  }
  // never reached here
  return E_NOTREACHABLE;
}

int M2XStreamClient::readStatusCode(bool closeClient) {
  int responseCode = 0;
  int ret = waitForString("HTTP/*.* ");
  if (ret != E_OK) {
    if (closeClient) close();
    return ret;
  }

  // ret is not needed from here(since it must be E_OK), so we can use it
  // as a regular variable now.
  ret = 0;
  while (true) {
    while (_client->available()) {
      char c = _client->read();
      DBG("%c", c);

      responseCode = responseCode * 10 + (c - '0');
      ret++;
      if (ret == 3) {
        if (closeClient) close();
        return responseCode;
      }
    }

    if (!_client->connected()) {
      DBGLN("%s", "ERROR: The client is disconnected from the server!");

      if (closeClient) close();
      return E_DISCONNECTED;
    }

    delay(1000);
  }

  // never reached here
  return E_NOTREACHABLE;
}

int M2XStreamClient::readContentLength() {
  int ret = waitForString("Content-Length: ");
  if (ret != E_OK) {
    return ret;
  }

  // From now on, ret is not needed, we can use it
  // to keep the final result
  ret = 0;
  while (true) {
    while (_client->available()) {
      char c = _client->read();
      DBG("%c", c);

      if ((c == '\r') || (c == '\n')) {
        return (ret == 0) ? (E_INVALID) : (ret);
      } else {
        ret = ret * 10 + (c - '0');
      }
    }

    if (!_client->connected()) {
      DBGLN("%s", "ERROR: The client is disconnected from the server!");

      return E_DISCONNECTED;
    }

    delay(1000);
  }

  // never reached here
  return E_NOTREACHABLE;
}

int M2XStreamClient::skipHttpHeader() {
  return waitForString("\r\n\r\n");
}

void M2XStreamClient::close() {
  // Eats up buffered data before closing
  _client->flush();
  _client->stop();
}

int M2XStreamClient::readStreamValue(stream_value_read_callback callback,
                                     void* context) {
  const int BUF_LEN = 32;
  char buf[BUF_LEN];

  int length = readContentLength();
  if (length < 0) {
    close();
    return length;
  }

  int index = skipHttpHeader();
  if (index != E_OK) {
    close();
    return index;
  }
  index = 0;

  stream_parsing_context_state state;
  state.state = state.index = 0;
  state.callback = callback;
  state.context = context;

  jsonlite_parser_callbacks cbs = jsonlite_default_callbacks;
  cbs.key_found = on_stream_key_found;
  cbs.string_found = on_stream_string_found;
  cbs.context.client_state = &state;

  jsonlite_parser p = jsonlite_parser_init(jsonlite_parser_estimate_size(5));
  jsonlite_parser_set_callback(p, &cbs);

  jsonlite_result result = jsonlite_result_unknown;
  while (index < length) {
    int i = 0;

    DBG("%s", "Received Data: ");
    while ((i < BUF_LEN) && _client->available()) {
      buf[i++] = _client->read();
      DBG("%c", buf[i - 1]);
    }
    DBGLNEND;

    if ((!_client->connected()) &&
        (!_client->available()) &&
        ((index + i) < length)) {
      jsonlite_parser_release(p);
      close();
      return E_NOCONNECTION;
    }

    result = jsonlite_parser_tokenize(p, buf, i);
    if ((result != jsonlite_result_ok) &&
        (result != jsonlite_result_end_of_stream)) {
      jsonlite_parser_release(p);
      close();
      return E_JSON_INVALID;
    }

    index += i;
  }

  jsonlite_parser_release(p);
  close();
  return (result == jsonlite_result_ok) ? (E_OK) : (E_JSON_INVALID);
}

int M2XStreamClient::readLocation(location_read_callback callback,
                                  void* context) {
  const int BUF_LEN = 40;
  char buf[BUF_LEN];

  int length = readContentLength();
  if (length < 0) {
    close();
    return length;
  }

  int index = skipHttpHeader();
  if (index != E_OK) {
    close();
    return index;
  }
  index = 0;

  location_parsing_context_state state;
  state.state = state.index = 0;
  state.callback = callback;
  state.context = context;

  jsonlite_parser_callbacks cbs = jsonlite_default_callbacks;
  cbs.key_found = on_location_key_found;
  cbs.string_found = on_location_string_found;
  cbs.context.client_state = &state;

  jsonlite_parser p = jsonlite_parser_init(jsonlite_parser_estimate_size(5));
  jsonlite_parser_set_callback(p, &cbs);

  jsonlite_result result = jsonlite_result_unknown;
  while (index < length) {
    int i = 0;

    DBG("%s", "Received Data: ");
    while ((i < BUF_LEN) && _client->available()) {
      buf[i++] = _client->read();
      DBG("%c", buf[i - 1]);
    }
    DBGLNEND;

    if ((!_client->connected()) &&
        (!_client->available()) &&
        ((index + i) < length)) {
      jsonlite_parser_release(p);
      close();
      return E_NOCONNECTION;
    }

    result = jsonlite_parser_tokenize(p, buf, i);
    if ((result != jsonlite_result_ok) &&
        (result != jsonlite_result_end_of_stream)) {
      jsonlite_parser_release(p);
      close();
      return E_JSON_INVALID;
    }

    index += i;
  }

  jsonlite_parser_release(p);
  close();
  return (result == jsonlite_result_ok) ? (E_OK) : (E_JSON_INVALID);
}
