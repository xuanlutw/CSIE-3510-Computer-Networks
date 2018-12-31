#ifndef COLOR_H
#define COLOR_H

#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#define FG_BLACK        "30"
#define FG_RED          "31"
#define FG_GREEN        "32"
#define FG_YELLOW       "33"
#define FG_BLUE         "34"
#define FG_PURPLE       "35"
#define FG_CYAN         "36"
#define FG_WHITE        "37"
#define FG_LIGHT_BLACK  "90"
#define FG_LIGHT_RED    "91"
#define FG_LIGHT_GREEN  "92"
#define FG_LIGHT_YELLOW "93"
#define FG_LIGHT_BLUE   "94"
#define FG_LIGHT_PURPLE "95"
#define FG_LIGHT_CYAN   "96"
#define FG_LIGHT_WHITE  "97"

#define BG_BLACK        "40"
#define BG_RED          "41"
#define BG_GREEN        "42"
#define BG_YELLOW       "43"
#define BG_BLUE         "44"
#define BG_PURPLE       "45"
#define BG_CYAN         "46"
#define BG_WHITE        "47"
#define BG_LIGHT_BLACK  "100"
#define BG_LIGHT_RED    "101"
#define BG_LIGHT_GREEN  "102"
#define BG_LIGHT_YELLOW "103"
#define BG_LIGHT_BLUE   "104"
#define BG_LIGHT_PURPLE "105"
#define BG_LIGHT_CYAN   "106"
#define BG_LIGHT_WHITE  "107"

#define BOLD            "1"
#define UNDERLINE       "4"
#define BLINK           "6"
#define RESET           "0"

#define set_feature(x) printf("\033[%sm", (x))


#define cusor_up(x)         printf("\033[%dA", (x));fflush(stdout)
#define cusor_down(x)       printf("\033[%dB", (x));fflush(stdout)
#define cusor_forward(x)    printf("\033[%dC", (x));fflush(stdout)
#define cusor_back(x)       printf("\033[%dD", (x));fflush(stdout)
#define cusor_next_l(x)     printf("\033[%dE", (x));fflush(stdout)
#define cusor_pre_l(x)      printf("\033[%dF", (x));fflush(stdout)
#define cusor_set_col(x)    printf("\033[%dG", (x));fflush(stdout)
#define cusor_set(x, y)     printf("\033[%d;%dH", (x), (y));fflush(stdout)
#define cusor_save()        printf("\033[s");fflush(stdout)
#define cusor_restore()     printf("\033[u");fflush(stdout)


#define erase_display_b()   printf("\033[0J");fflush(stdout)
#define erase_display_f()   printf("\033[1J");fflush(stdout)
#define erase_display_all() printf("\033[3J");fflush(stdout)
#define erase_display_al()  printf("\033[2J");fflush(stdout)   // no erase catch
#define erase_line_b()      printf("\033[0K");fflush(stdout)
#define erase_line_f()      printf("\033[1K");fflush(stdout)
#define erase_line_all()    printf("\033[2K");fflush(stdout)
#define scroll_up()         printf("\033[S");fflush(stdout)
#define scroll_down()       printf("\033[T");fflush(stdout)

#endif
