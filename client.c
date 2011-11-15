#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "client.h"
#include "player.h"
#include "ui.h"
#include "werld_client.h"

#ifdef WERLD_DEVELOPMENT
static const char *WERLD_SERVER_ADDRESS = "0.0.0.0";
#else
static const char *WERLD_SERVER_ADDRESS = "server.werldonline.com";
#endif
static const char *WERLD_SERVER_PORT = "9876";

static const char *WERLD_REQUEST_PLAYER     = "player";
static const char *WERLD_REQUEST_PLAYERS    = "players";
static const char *WERLD_REQUEST_REGISTER   = "register";
static const char *WERLD_REQUEST_UNREGISTER = "unregister";
static const char *WERLD_REQUEST_MESSAGE    = "message";

static const size_t WERLD_RESPONSE_TYPE_BUFSIZ = 4;

enum { WERLD_RESPONSE_TYPE_ERROR = -1,
       WERLD_RESPONSE_TYPE_REGISTER,
       WERLD_RESPONSE_TYPE_PLAYERS,
       WERLD_RESPONSE_TYPE_MESSAGE };

#define WERLD_REQUEST_PLAYER_BUFSIZ (strlen(WERLD_REQUEST_PLAYER) + \
                                     sizeof(struct player))

#define WERLD_REQUEST_REGISTER_BUFSIZ (strlen(WERLD_REQUEST_REGISTER) + \
                                       sizeof(struct player))

#define WERLD_REQUEST_UNREGISTER_BUFSIZ (strlen(WERLD_REQUEST_UNREGISTER) + \
                                         sizeof(struct player))

#define WERLD_REQUEST_MESSAGE_BUFSIZ (strlen(WERLD_REQUEST_MESSAGE) + \
                                      sizeof(struct player) + \
                                      WERLD_PLAYER_MESSAGE_BUFSIZ)

#define WERLD_REQUEST_MESSAGE_SIZE(message) (strlen(WERLD_REQUEST_MESSAGE) + \
                                             sizeof(struct player) + \
                                             strlen(message))

#define WERLD_RESPONSE_REGISTER_BUFSIZ sizeof(struct player)

static void client_register(struct player player) {
  ssize_t bytes_written;
  char data[WERLD_REQUEST_REGISTER_BUFSIZ];

  void *offset =
    mempcpy(data, WERLD_REQUEST_REGISTER, strlen(WERLD_REQUEST_REGISTER));
  memcpy(offset, &player, sizeof(struct player));

  if ((bytes_written = write(werld_client.fd, data, sizeof(data))) < 0) {
    perror("write");
    exit(errno);
  }
  werld_client_log_binary(WERLD_CLIENT_DEBUG,
                          data,
                          "+register bytes written: %zd ",
                          bytes_written);
}

int client_connect(struct player player) {
  int status;
  struct addrinfo hints;
  struct addrinfo *results;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(WERLD_SERVER_ADDRESS,
                            WERLD_SERVER_PORT,
                            &hints,
                            &results))) {
    werld_client_log(WERLD_CLIENT_ERROR, "getaddrinfo: %s\n", gai_strerror(status));
    exit(errno);
  }

  struct addrinfo *iterator;

  for (iterator = results; iterator; iterator = iterator->ai_next) {
    if ((werld_client.fd = socket(iterator->ai_family,
                                  iterator->ai_socktype,
                                  iterator->ai_protocol)) == -1) {
      continue;
    }

    if (connect(werld_client.fd, iterator->ai_addr, iterator->ai_addrlen) == -1) {
      close(werld_client.fd);
      continue;
    }

    break;
  }

  freeaddrinfo(results);

  if (iterator) {
    client_register(player);
    return(0);
  }

  return(-1);
}

int client_disconnect(struct player player) {
  ssize_t bytes_written;
  char data[WERLD_REQUEST_UNREGISTER_BUFSIZ];

  void *offset =
    mempcpy(data, WERLD_REQUEST_UNREGISTER, strlen(WERLD_REQUEST_UNREGISTER));
  memcpy(offset, &player, sizeof(player));

  if ((bytes_written = write(werld_client.fd, &data, sizeof(data))) < 0) {
    perror("write");
    exit(errno);
  }

  werld_client_log_binary(WERLD_CLIENT_DEBUG,
                          data,
                          "+disconnect bytes written: %zd ",
                          bytes_written);

  if (close(werld_client.fd)) {
    perror("close");
    exit(errno);
  }

  return(0);
}

int client_send_player(struct player player) {
  ssize_t bytes_written;
  char data[WERLD_REQUEST_PLAYER_BUFSIZ];

  void *offset =
    mempcpy(data, WERLD_REQUEST_PLAYER, strlen(WERLD_REQUEST_PLAYER));
  memcpy(offset, &player, sizeof(player));

  if ((bytes_written = write(werld_client.fd, &data, sizeof(data))) < 0) {
    perror("write");
    exit(errno);
  }

  werld_client_log_binary(WERLD_CLIENT_DEBUG,
                          data,
                          "+send_player bytes written: %zd ",
                          bytes_written);

  return(0);
}

int client_request_players(void) {
  ssize_t bytes_written;

  if ((bytes_written = write(werld_client.fd,
                             WERLD_REQUEST_PLAYERS,
                             strlen(WERLD_REQUEST_PLAYERS))) < 0) {
    perror("write");
    exit(errno);
  }

  werld_client_log(WERLD_CLIENT_DEBUG,
                   "+request_players bytes written: %zd ",
                   bytes_written);

  return(0);
}

int client_handle_response(void) {
  ssize_t bytes_read;
  uint32_t response_type;

  if ((bytes_read = read(werld_client.fd,
                         &response_type,
                         WERLD_RESPONSE_TYPE_BUFSIZ)) < 0) {
    perror("read");
    exit(errno);
  }

  if (bytes_read == 0) {
    return(-1);
  }

  if (response_type == WERLD_RESPONSE_TYPE_REGISTER) {
    char data[WERLD_RESPONSE_REGISTER_BUFSIZ];
    struct player player;

    if ((bytes_read = read(werld_client.fd,
                           &player,
                           WERLD_RESPONSE_REGISTER_BUFSIZ)) < 0) {
      perror("read");
      exit(errno);
    }

    if (bytes_read == 0) return(-1);

    werld_client_log_binary(WERLD_CLIENT_DEBUG,
                            data,
                            "+handle_response+register bytes read: %zd ",
                            bytes_read);

    memcpy(werld_client.player, &player, sizeof(struct player));
    ui_draw_player(*(werld_client.player));
    refresh();
  } else if (response_type == WERLD_RESPONSE_TYPE_MESSAGE) {
    uint32_t message_length;

    if ((bytes_read = read(werld_client.fd, &message_length, sizeof(uint32_t))) < 0) {
      perror("read");
      exit(errno);
    }

    if (bytes_read == 0) {
      return(-1);
    }

    werld_client_log_binary(WERLD_CLIENT_DEBUG,
                            (char *) &message_length,
                            "+handle_response+message bytes read: %zd ",
                            bytes_read);

    char message[message_length + 1];
    char payload[sizeof(message) + sizeof(struct player)];

    if ((bytes_read = read(werld_client.fd, payload, sizeof(payload))) < 0) {
      perror("read");
      exit(errno);
    }

    if (bytes_read == 0) {
      return(-1);
    }

    werld_client_log_binary(WERLD_CLIENT_DEBUG,
                            payload,
                            "+handle_response+message bytes read: %zd ",
                            bytes_read);

    struct player player;

    memcpy(&player, payload, sizeof(struct player));
    strncpy(message, payload + sizeof(struct player), sizeof(message));

    ui_draw_player_with_message(player, message);
    refresh();
  } else if (response_type == WERLD_RESPONSE_TYPE_PLAYERS) {
    uint32_t number_of_players;

    if ((bytes_read = read(werld_client.fd, &number_of_players, sizeof(uint32_t))) < 0) {
      perror("read");
      exit(errno);
    }

    if (bytes_read == 0) {
      return(-1);
    }

    werld_client_log_binary(WERLD_CLIENT_DEBUG,
                            (char *) &number_of_players,
                            "+handle_response+players bytes read: %zd ",
                            bytes_read);

    char payload[number_of_players * sizeof(struct player)];

    if ((bytes_read = read(werld_client.fd, payload, sizeof(payload))) < 0) {
      perror("read");
      exit(errno);
    }

    if (bytes_read == 0) {
      return(-1);
    }

    werld_client_log_binary(WERLD_CLIENT_DEBUG,
                            payload,
                            "+handle_response+players bytes read: %zd ",
                            bytes_read);

    ui_erase_player_list(werld_client.player_list);
    player_list_update(&(werld_client.player_list), (void *) payload, number_of_players);
    ui_draw_player_list(werld_client.player_list);
    refresh();
  }

  return(0);
}

int client_send_message(struct player player, const char *message) {
  char data[WERLD_REQUEST_MESSAGE_SIZE(message)];
  ssize_t bytes_written;

  void *offset =
    mempcpy(data, WERLD_REQUEST_MESSAGE, strlen(WERLD_REQUEST_MESSAGE));
  /* FIXME: Don't send entire player. */
  offset = mempcpy(offset, &player, sizeof(player));
  memcpy(offset, message, strlen(message));

  if ((bytes_written = write(werld_client.fd,
                             &data,
                             WERLD_REQUEST_MESSAGE_SIZE(message))) < 0) {
    perror("write");
    exit(errno);
  }

  werld_client_log_binary(WERLD_CLIENT_DEBUG,
                          data,
                          "+send_message bytes written: %zd ",
                          bytes_written);

  return(0);
}
