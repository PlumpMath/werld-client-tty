#include <curses.h>
#include <stdio.h>
#include <stdlib.h>

#include "client.h"
#include "movement.h"
#include "player.h"
#include "player_list.h"

#define MESSAGE_INPUT_Y 24
#define MESSAGE_INPUT_X 0

void keyboard_event(int key) {
  char message[MESSAGE_BUFSIZ];

  switch (key) {
  case 'q':
    client_disconnect(player);
    player_list_free(player_list);
    endwin();
    exit(0);
  case '\n':
    echo();
    mvwgetnstr(stdscr,
               MESSAGE_INPUT_Y,
               MESSAGE_INPUT_X,
               message,
               sizeof(message) - 1);
    client_send_message(player, message);
    noecho();
    break;
  case 'h':
  case 'j':
  case 'k':
  case 'l':
  case KEY_LEFT:
  case KEY_DOWN:
  case KEY_UP:
  case KEY_RIGHT:
    player_move(&player, movement_direction(key));
    break;
  default:
    break;
  }
}
