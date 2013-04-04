// Necklace of the Eye v6.2
// roguelike frontend
// Copyright (C) 2010-2012 Zeno Rogue, see 'noteye.h' for details

// include this file instead of Curses if you want to integrate it with NotEye

#define KEY_F0 256
#define NOTEYEERR (-256) // returned if there was an error with NotEye

#define DBASE (512*6)

#define D_RIGHT (DBASE+0)
#define D_UP    (DBASE+2)
#define D_LEFT  (DBASE+4)
#define D_DOWN  (DBASE+6)
#define D_PGUP  (DBASE+1)
#define D_PGDN  (DBASE+7)
#define D_HOME  (DBASE+3)
#define D_END   (DBASE+5)
#define D_CTR   (DBASE+8)

// note: Ctrl/Shift modifiers 

#ifdef CURSES_CONSTONLY
#define _NOTEYE_CURSES_H
#endif

#ifndef _NOTEYE_CURSES_H
#define _NOTEYE_CURSES_H

#define addstr    noteye_addstr
#define addch     noteye_addch
#define refresh   noteye_refresh
#define clrtoeol  noteye_clrtoeol
#define move      noteye_move
#define erase     noteye_erase
#define endwin    noteye_endwin
#define halfdelay noteye_halfdelay
#define cbreak    noteye_cbreak

typedef struct lua_State lua_State;

// extern "C" {
  // these simply replace Curses functions
  void noteye_addch(char ch);
  void noteye_addchx(int ch); // if more than 256 chars
  void noteye_addstr(const char *buf);
  void noteye_endwin();
  void noteye_cbreak();
  void noteye_halfdelay(int i);
  void noteye_erase();
  void noteye_move(int y, int x);
  void noteye_clrtoeol();
  void noteye_refresh();

  // use these instead of Curses functions
  void initScreen();
  
  // set foreground color to 'fore' and background color to 'back',
  // where colors are numbered 0-15, just like in DOS
  void setTextAttr(int fore, int back);

  // in this variant, colors are given like 0xFRRGGBB, where
  // RRGGBB is TrueColor value of the color, and F is the DOS
  // equivalent used on console
  void setTextAttr32(int fore, int back);

  int noteye_getch();
  // getchev will also report non-printable keypresses and key releases,
  // as 0
  int noteye_getchev();
  // report the full information about the last character seen by
  // getch or getchev, as SDL_Event
  union SDL_Event *noteye_getlastkeyevent();

  void noteye_mvaddch(int y, int x, char ch);
  void noteye_mvaddchx(int y, int x, int ch);
  void noteye_mvaddstr(int y, int x, const char *buf);

  // translate the DOS color index to FRRGGBB
  int getVGAcolor(int c);

  // you can use these to interact with Lua
  void setfield (lua_State *L, const char *index, int value);
  void setfieldString (lua_State *L, const char *index, const char *s);
  void setfieldBool (lua_State *L, const char *index, bool b);
  void noteye_lua_newtable(lua_State *L);

#define NOCURSES
#endif
