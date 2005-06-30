#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <panel.h>
#include <time.h>
#include <ctype.h>

#include "screen.h"
#include "utils.h"
#include "buddies.h"
#include "parsecfg.h"
#include "lang.h"
#include "server.h"

#include "list.h"

/* Definicion de tipos */
#define window_entry(n) list_entry(n, window_entry_t, list)

typedef struct _window_entry_t {
  WINDOW *win;
  PANEL *panel;
  char *name;
  int nlines;
  char **texto;
  struct list_head list;
} window_entry_t;

LIST_HEAD(window_list);

/* Variables globales a SCREEN.C */
WINDOW *rosterWnd, *chatWnd;
PANEL *rosterPanel, *chatPanel;
int maxY, maxX;
window_entry_t *ventanaActual;


/* Funciones */

int scr_WindowHeight(WINDOW * win)
{
  int x, y;
  getmaxyx(win, y, x);
  return x;
}

void
scr_draw_box(WINDOW * win, int y, int x, int height, int width, int Color,
	     chtype box, chtype border)
{
  int i, j;

  wattrset(win, COLOR_PAIR(Color));
  for (i = 0; i < height; i++) {
    wmove(win, y + i, x);
    for (j = 0; j < width; j++)
      if (!i && !j)
	waddch(win, border | ACS_ULCORNER);
      else if (i == height - 1 && !j)
	waddch(win, border | ACS_LLCORNER);
      else if (!i && j == width - 1)
	waddch(win, box | ACS_URCORNER);
      else if (i == height - 1 && j == width - 1)
	waddch(win, box | ACS_LRCORNER);
      else if (!i)
	waddch(win, border | ACS_HLINE);
      else if (i == height - 1)
	waddch(win, box | ACS_HLINE);
      else if (!j)
	waddch(win, border | ACS_VLINE);
      else if (j == width - 1)
	waddch(win, box | ACS_VLINE);
      else
	waddch(win, box | ' ');
  }
}

int FindColor(char *name)
{
  if (!strcmp(name, "default"))
    return -1;
  if (!strcmp(name, "black"))
    return COLOR_BLACK;
  if (!strcmp(name, "red"))
    return COLOR_RED;
  if (!strcmp(name, "green"))
    return COLOR_GREEN;
  if (!strcmp(name, "yellow"))
    return COLOR_YELLOW;
  if (!strcmp(name, "blue"))
    return COLOR_BLUE;
  if (!strcmp(name, "magenta"))
    return COLOR_MAGENTA;
  if (!strcmp(name, "cyan"))
    return COLOR_CYAN;
  if (!strcmp(name, "white"))
    return COLOR_WHITE;

  return -1;
}

void ParseColors(void)
{
  char *colors[11] = {
    "", "",
    "borderlines",
    "jidonlineselected",
    "jidonline",
    "jidofflineselected",
    "jidoffline",
    "text",
    NULL
  };

  char *tmp = malloc(1024);
  char *color1;
  char *background = cfg_read("color_background");
  char *backselected = cfg_read("color_backselected");
  int i = 0;

  while (colors[i]) {
    sprintf(tmp, "color_%s", colors[i]);
    color1 = cfg_read(tmp);

    switch (i + 1) {
    case 1:
      init_pair(1, COLOR_BLACK, COLOR_WHITE);
      break;
    case 2:
      init_pair(2, COLOR_WHITE, COLOR_BLACK);
      break;
    case 3:
      init_pair(3, FindColor(color1), FindColor(background));
      break;
    case 4:
      init_pair(4, FindColor(color1), FindColor(backselected));
      break;
    case 5:
      init_pair(5, FindColor(color1), FindColor(background));
      break;
    case 6:
      init_pair(6, FindColor(color1), FindColor(backselected));
      break;
    case 7:
      init_pair(7, FindColor(color1), FindColor(background));
      break;
    case 8:
      init_pair(8, FindColor(color1), FindColor(background));
      break;
    }
    i++;
  }
}


window_entry_t *scr_CreatePanel(char *title, int x, int y, int lines,
				int cols)
{
  window_entry_t *tmp = calloc(1, sizeof(window_entry_t));

  tmp->win = newwin(lines, cols, y, x);
  tmp->panel = new_panel(tmp->win);
  tmp->name = (char *) calloc(1, 1024);
  strncpy(tmp->name, title, 1024);
  scr_draw_box(tmp->win, 0, 0, lines, cols, COLOR_GENERAL, 0, 0);
  mvwprintw(tmp->win, 0, (cols - (2 + strlen(title))) / 2, " %s ", title);

  list_add_tail(&tmp->list, &window_list);
  update_panels();

  return tmp;
}


void
scr_CreatePopup(char *title, char *texto, int corte, int type,
		char *returnstring)
{
  WINDOW *popupWin;
  PANEL *popupPanel;

  int lineas = 0;
  int cols = 0;

  char **submsgs;
  int n = 0;
  int i;

  char *instr = (char *) calloc(1, 1024);

  /* fprintf(stderr, "\r\n%d", lineas); */

  submsgs = ut_SplitMessage(texto, &n, corte);

  switch (type) {
  case 1:
  case 0:
    lineas = n + 4;
    break;
  }

  cols = corte + 3;
  popupWin = newwin(lineas, cols, (maxY - lineas) / 2, (maxX - cols) / 2);
  popupPanel = new_panel(popupWin);

  /*ATENCION!!! Colorear el popup ??
     / box (popupWin, 0, 0); */
  scr_draw_box(popupWin, 0, 0, lineas, cols, COLOR_POPUP, 0, 0);
  mvwprintw(popupWin, 0, (cols - (2 + strlen(title))) / 2, " %s ", title);

  for (i = 0; i < n; i++)
    mvwprintw(popupWin, i + 1, 2, "%s", submsgs[i]);


  for (i = 0; i < n; i++)
    free(submsgs[i]);
  free(submsgs);

  switch (type) {
  case 0:
    mvwprintw(popupWin, n + 2,
	      (cols - (2 + strlen(i18n("Press any key")))) / 2,
	      i18n("Press any key"));
    update_panels();
    doupdate();
    getch();
    break;
  case 1:
    {
      char ch;
      int scroll = 0;
      int input_x = 0;

      wmove(popupWin, 3, 1);
      wrefresh(popupWin);
      keypad(popupWin, TRUE);
      while ((ch = getch()) != '\n') {
	switch (ch) {
	case 0x09:
	case KEY_UP:
	case KEY_DOWN:
	  break;
	case KEY_RIGHT:
	case KEY_LEFT:
	  break;
	case KEY_BACKSPACE:
	case 127:
	  if (input_x || scroll) {
	    /* wattrset (popupWin, 0); */
	    if (!input_x) {
	      scroll = scroll < cols - 3 ? 0 : scroll - (cols - 3);
	      wmove(popupWin, 3, 1);
	      for (i = 0; i < cols; i++)
		waddch
		    (popupWin,
		     instr
		     [scroll
		      + input_x + i] ? instr[scroll + input_x + i] : ' ');
	      input_x = strlen(instr) - scroll;
	    } else
	      input_x--;
	    instr[scroll + input_x] = '\0';
	    mvwaddch(popupWin, 3, input_x + 1, ' ');
	    wmove(popupWin, 3, input_x + 1);
	    wrefresh(popupWin);
	  }
	default:
	  if ( /*ch<0x100 && */ isprint(ch) || ch == '�'
	      || ch == '�') {
	    if (scroll + input_x < 1024) {
	      instr[scroll + input_x] = ch;
	      instr[scroll + input_x + 1] = '\0';
	      if (input_x == cols - 3) {
		scroll++;
		wmove(popupWin, 3, 1);
		for (i = 0; i < cols - 3; i++)
		  waddch(popupWin, instr[scroll + i]);
	      } else {
		wmove(popupWin, 3, 1 + input_x++);
		waddch(popupWin, ch);
	      }
	      wrefresh(popupWin);
	    } else {
	      flash();
	    }
	  }
	}
      }
    }
    if (returnstring != NULL)
      strcpy(returnstring, instr);
    break;
  }

  del_panel(popupPanel);
  delwin(popupWin);
  update_panels();
  doupdate();
  free(instr);
  keypad(rosterWnd, TRUE);
}



void scr_RoolWindow(void)
{
}

window_entry_t *scr_SearchWindow(char *nombreVentana)
{
  struct list_head *pos, *n;
  window_entry_t *search_entry = NULL;

  list_for_each_safe(pos, n, &window_list) {
    search_entry = window_entry(pos);
    if (search_entry->name) {
      if (!strcasecmp(search_entry->name, nombreVentana)) {
	return search_entry;
      }
    }
  }
  return NULL;
}

void scr_ShowWindow(char *nombreVentana)
{
  int n, width, i;
  window_entry_t *tmp = scr_SearchWindow(nombreVentana);
  if (tmp != NULL) {
    top_panel(tmp->panel);
    width = scr_WindowHeight(tmp->win);
    for (n = 0; n < tmp->nlines; n++) {
      mvwprintw(tmp->win, n + 1, 1, "");
      for (i = 0; i < width - 2; i++)
	waddch(tmp->win, ' ');
      mvwprintw(tmp->win, n + 1, 1, "%s", tmp->texto[n]);
    }
    move(maxY - 2, maxX - 1);
    update_panels();
    doupdate();
  }
}

void scr_ShowBuddyWindow(void)
{
  buddy_entry_t *tmp = bud_SelectedInfo();
  if (tmp->jid != NULL)
    scr_ShowWindow(tmp->jid);
}


void scr_WriteInWindow(char *nombreVentana, char *texto, int TimeStamp)
{
  time_t ahora;
  int n;
  int i;
  int width;
  window_entry_t *tmp;

  tmp = scr_SearchWindow(nombreVentana);
  if (tmp == NULL) {
    tmp = scr_CreatePanel(nombreVentana, 20, 0, maxY-1, maxX - 20);
    tmp->texto = (char **) calloc(maxY * 3, sizeof(char *));
    for (n = 0; n < (maxY-1) * 3; n++)
      tmp->texto[n] = (char *) calloc(1, 1024);

    if (TimeStamp) {
      ahora = time(NULL);
      strftime(tmp->texto[tmp->nlines], 1024, "[%H:%M] ",
	       localtime(&ahora));
      strcat(tmp->texto[tmp->nlines], texto);
    } else {
      sprintf(tmp->texto[tmp->nlines], "            %s", texto);
    }
    tmp->nlines++;
  } else {
    if (tmp->nlines < maxY - 3) {
      if (TimeStamp) {
	ahora = time(NULL);
	strftime(tmp->texto[tmp->nlines], 1024,
		 "[%H:%M] ", localtime(&ahora));
	strcat(tmp->texto[tmp->nlines], texto);
      } else {
	sprintf(tmp->texto[tmp->nlines], "            %s", texto);
      }
      tmp->nlines++;
    } else {
      for (n = 0; n < tmp->nlines; n++) {
	memset(tmp->texto[n], 0, 1024);
	strncpy(tmp->texto[n], tmp->texto[n + 1], 1024);
      }
      if (TimeStamp) {
	ahora = time(NULL);
	strftime(tmp->texto[tmp->nlines - 1], 1024,
		 "[%H:%M] ", localtime(&ahora));
	strcat(tmp->texto[tmp->nlines - 1], texto);
      } else {
	sprintf(tmp->texto[tmp->nlines - 1], "            %s", texto);
      }
    }
  }

  top_panel(tmp->panel);
  width = scr_WindowHeight(tmp->win);
  for (n = 0; n < tmp->nlines; n++) {
    mvwprintw(tmp->win, n + 1, 1, "");
    for (i = 0; i < width - 2; i++)
      waddch(tmp->win, ' ');
    mvwprintw(tmp->win, n + 1, 1, "%s", tmp->texto[n]);
  }

  update_panels();
  doupdate();
}

void scr_InitCurses(void)
{
  initscr();
  noecho();
  raw();
  start_color();
  use_default_colors();

  ParseColors();

  getmaxyx(stdscr, maxY, maxX);

  return;
}

void scr_DrawMainWindow(void)
{
  /* Dibujamos los paneles principales */
  rosterWnd = newwin(maxY-1, 20, 0, 0);
  rosterPanel = new_panel(rosterWnd);
  scr_draw_box(rosterWnd, 0, 0, maxY-1, 20, COLOR_GENERAL, 0, 0);
  mvwprintw(rosterWnd, 0, (20 - strlen(i18n("Roster"))) / 2,
	    i18n("Roster"));

  chatWnd = newwin(maxY-1, maxX - 20, 0, 20);
  chatPanel = new_panel(chatWnd);
  scr_draw_box(chatWnd, 0, 0, maxY-1, maxX - 20, COLOR_GENERAL, 0, 0);
  mvwprintw(chatWnd, 0,
	    ((maxX - 20) - strlen(i18n("Status Window"))) / 2,
	    i18n("Status Window"));

  bud_DrawRoster(rosterWnd);

  update_panels();
  doupdate();
  return;
}

void scr_TerminateCurses(void)
{
  clear();
  refresh();
  endwin();
  return;
}

void scr_WriteIncomingMessage(char *jidfrom, char *text)
{
  char **submsgs;
  int n, i;
  char *buffer = (char *) malloc(5 + strlen(text));

  sprintf(buffer, "<<< %s", text);

  submsgs =
      ut_SplitMessage(buffer, &n, maxX - scr_WindowHeight(rosterWnd) - 20);

  for (i = 0; i < n; i++) {
    if (i == 0)
      scr_WriteInWindow(jidfrom, submsgs[i], TRUE);
    else
      scr_WriteInWindow(jidfrom, submsgs[i], FALSE);
  }

  for (i = 0; i < n; i++)
    free(submsgs[i]);

  free(submsgs);
  free(buffer);

}

void scr_WriteMessage(int sock)
{
  char **submsgs;
  int n, i;
  char *buffer = (char *) calloc(1, 1024);
  char *buffer2 = (char *) calloc(1, 1024);
  buddy_entry_t *tmp = bud_SelectedInfo();

  scr_ShowWindow(tmp->jid);

  ut_CenterMessage(i18n("write your message here"), 60, buffer2);

  scr_CreatePopup(tmp->jid, buffer2, 60, 1, buffer);

  if (strlen(buffer)) {
    sprintf(buffer2, ">>> %s", buffer);

    submsgs =
	ut_SplitMessage(buffer2, &n,
			maxX - scr_WindowHeight(rosterWnd) - 20);
    for (i = 0; i < n; i++) {
      if (i == 0)
	scr_WriteInWindow(tmp->jid, submsgs[i], TRUE);
      else
	scr_WriteInWindow(tmp->jid, submsgs[i], FALSE);
    }

    for (i = 0; i < n; i++)
      free(submsgs[i]);
    free(submsgs);

    move(maxY - 2, maxX - 1);
    refresh();
    sprintf(buffer2, "%s@%s/%s", cfg_read("username"),
	    cfg_read("server"), cfg_read("resource"));
    srv_sendtext(sock, tmp->jid, buffer, buffer2);
  }
  free(buffer);
  free(buffer2);
}

int scr_Getch(void)
{
  int ch;
  keypad(rosterWnd, TRUE);
  ch = wgetch(rosterWnd);
  return ch;
}

WINDOW *scr_GetRosterWindow(void)
{
  return rosterWnd;
}

WINDOW *scr_GetStatusWindow(void)
{
  return chatWnd;
}
