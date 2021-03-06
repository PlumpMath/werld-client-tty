#ifndef WERLD_WERLD_CLIENT_H
#define WERLD_WERLD_CLIENT_H

#include <curses.h>
#include <stdarg.h>

#include "map.h"
#include "player.h"
#include "player_list.h"

enum { WERLD_CLIENT_DEBUG, WERLD_CLIENT_ERROR, WERLD_CLIENT_INFO };

struct werld_client {
  WINDOW *main_window;
  WINDOW *message_bar_window;
  WINDOW *status_bar_window;
  WINDOW *world_map_window;
  char *log_file;
  double player_messages_lifetime;
  int fd;
  int log_level;
  int message_handler_fds[2];
  struct map *world_map;
  struct player *player;
  struct player_list *player_list;
};

extern struct werld_client werld_client;

void werld_client_log(int, const char *, ...);
void werld_client_log_binary(int, const uint8_t *, size_t, char *, ...);
void werld_client_init(struct werld_client *);
void werld_client_kill(struct werld_client *);

#endif
