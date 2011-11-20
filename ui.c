#include <curses.h>
#include <string.h>

#include "message_list.h"
#include "player.h"
#include "player_list.h"
#include "werld_client.h"

void ui_draw_player(struct player player) {
  move(player.y - 1, (player.x - strlen(player.name) / 2) -1);
  addch('(');
  addstr(player.name);
  addch(')');
  mvaddch(player.y, player.x, '@');
}

void ui_erase_player(struct player player) {
  move(player.y - 1, (player.x - strlen(player.name) / 2) -1);
  for (unsigned int i = 0; i < strlen(player.name) + 2; i++) {
    addch(' ');
  }
  move(player.y, player.x);
  addch(' ');
}

void ui_draw_player_list(const struct player_list *player_list) {
  for (; player_list; player_list = player_list->next) {
    ui_draw_player(*(player_list->player));
  }
}

void ui_erase_player_list(const struct player_list *player_list) {
  for (; player_list; player_list = player_list->next) {
    ui_erase_player(*(player_list->player));
  }
}

/* FIXME: refactor and don't depend on werld_client.player_list. */
void ui_draw_player_message_list(const struct player *player) {
  int y, x;
  struct message_list *message_list;
  struct player_list *player_list = werld_client.player_list;

  if (!player) return;

  message_list = NULL;
  for (; player_list; player_list = player_list->next) {
    if (player_list->player && player_list->player->id == player->id) {
      message_list = player_list->message_list;
    }
  }

  y = player->y;
  x = player->x;

  for (; message_list; message_list = message_list->next) {
    mvaddstr(y - 3,
             x - strlen(message_list->message) / 2,
             message_list->message);
    y--;
  }
}

/* FIXME: refactor and don't depend on werld_client.player_list. */
void ui_erase_player_message_list(const struct player *player) {
  int y, x;
  struct message_list *message_list;
  struct player_list *player_list = werld_client.player_list;

  if (!player) return;

  message_list = NULL;
  for (; player_list; player_list = player_list->next) {
    if (player_list->player && player_list->player->id == player->id) {
      message_list = player_list->message_list;
    }
  }

  y = player->y;
  x = player->x;

  for (; message_list; message_list = message_list->next) {
    move(y - 3, x - strlen(message_list->message) / 2);
    for (size_t i = 0; i < strlen(message_list->message); i++) {
      addch(' ');
    }
    y--;
  }
}
