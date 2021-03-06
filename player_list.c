#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "player_list.h"
#include "ui.h"

void player_list_malloc(struct player_list **player_list) {
  if (!(*player_list = malloc(sizeof(struct player_list)))) {
    perror("malloc");
    exit(errno);
  }
}

static void player_list_init(struct player_list **player_list) {
  *player_list = NULL;
}

void player_list_free(struct player_list **player_list) {
  struct player_list *tmp;
  struct player_list *iterator;

  iterator = *player_list;

  while (iterator) {
    tmp = iterator;
    if (iterator->player) player_free(&(iterator->player));
    message_list_free(&(iterator->message_list));
    iterator = iterator->next;
    free(tmp);
  }

  *player_list = NULL;
}

static bool player_list_player_member(const struct player_list *player_list,
                                      const struct player *player) {
  for (; player_list; player_list = player_list->next) {
    if (player_list->player && player_list->player->id == player->id) {
      return(true);
    }
  }

  return(false);
}

static void player_list_add_player(struct player_list **player_list,
                                   const struct player *player) {
  struct player_list **new;
  struct player_list *iterator = *player_list;
  struct player_list *tmp;

  for (; iterator; iterator = iterator->next) {
    tmp = iterator;
  }

  new = *player_list ? &(tmp->next) : player_list;

  player_list_malloc(new);
  player_malloc(&((*new)->player));
  memcpy((*new)->player, player, sizeof(struct player));
  message_list_init(&((*new)->message_list));
  (*new)->next = NULL;
}

static void player_list_update_player(struct player_list **player_list,
                                      const struct player *player) {
  struct player_list *iterator = *player_list;

  for (; iterator; iterator = iterator->next) {
    if (iterator->player && iterator->player->id == player->id) {
      memcpy(iterator->player, player, sizeof(struct player));
      return;
    }
  }
}

static int player_list_remove_player(struct player_list **player_list,
                                     const struct player *player) {
  struct player_list *iterator = *player_list;
  struct player_list *previous = NULL;

  if (!*player_list) return(0);

  for (; iterator; iterator = iterator->next) {
    if (iterator->player && iterator->player->id == player->id) {
      if (previous) {
        previous->next = iterator->next;
      }
      iterator = NULL;
      return(1);
    }
    previous = iterator;
  }

  return(0);
}

int player_list_add_message(struct player_list **player_list,
                            const char *message,
                            uint32_t player_id) {
  struct player_list *iterator = *player_list;
  time_t now;

  if (!*player_list) return(0);

  for (; iterator; iterator = iterator->next) {
    if (iterator->player && iterator->player->id == player_id) {
      time(&now);
      return(message_list_add(&(iterator->message_list), message, now));
    }
  }

  return(0);
}

int player_list_remove_message(struct player_list **player_list,
                               const char *message,
                               uint32_t player_id) {
  struct player_list *iterator = *player_list;

  for (; iterator; iterator = iterator->next) {
    if (iterator->player && iterator->player->id == player_id) {
      return(message_list_remove(&(iterator->message_list), message));
    }
  }

  return(0);
}

void player_list_update(struct player_list **player_list,
                        const struct player *players,
                        int number_of_players) {
  struct player_list *new_player_list;
  struct player_list *iterator;

  if (number_of_players <= 0) return;

  /* FIXME: in the future, when a player moves the server will not respond with the
   * entire player list, so we'll be able to refresh the moving player's message
   * list more intelligently than erasing and drawing the list for every player
   * in the list.
   */
  for (iterator = *player_list; iterator; iterator = iterator->next) {
    ui_erase_player_message_list(iterator->player);
  }

  player_list_init(&new_player_list);

  for (int i = 0; i < number_of_players; i++) {
    player_list_add_player(&new_player_list, &players[i]);
  }

  /* This is what we do in the two traversals below:
   *
   * member(old) && member(new) -> update
   * member(old) && !member(new) -> remove
   * !member(old) && member(new) -> add
   *
   * I know there are better data structures for the local players state. This
   * will be taken care of in the future (or not).
   */
  for (iterator = new_player_list; iterator; iterator = iterator->next) {
    if (player_list_player_member(*player_list, iterator->player)) {
      player_list_update_player(player_list, iterator->player);
    } else {
      player_list_add_player(player_list, iterator->player);
    }
    /* See FIXME above related to the server responding with the entire player
     * list when a client send its state.
     * */
    ui_draw_player_message_list(iterator->player);
  }

  for (iterator = *player_list; iterator; iterator = iterator->next) {
    if (!player_list_player_member(new_player_list, iterator->player)) {
      player_list_remove_player(player_list, iterator->player);
    }
  }
}
