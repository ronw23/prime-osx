// A sample roguelike using NotEye as a library.
// See also "games/sample.noe" for the very basic NotEye game configuration.
// It just does nothing -- just uses the defaults and sets the map region.
// See other configs ("games/*.noe") to know how to do things.

#include "noteye.h"
#include "noteye-curses.h"

int pcx = 40, pcy = 12;

int dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
int dy[8] = {0, -1, -1, -1, 0, 1, 1, 1};

// this function runs the "game"

void rungame() {

  // Actually, noteye_curses.h defines abbreviations, so you can
  // simply write "move" as in Curses instead of noteye_move
  // however, we use full names in this sample.

  // 10 = light green in DOS, 0 = black background
  setTextAttr(10, 0);
  noteye_move(0, 25);
  noteye_addstr("Welcome to Sample Roguelike!");
  noteye_move(23, 15);
  noteye_addstr("Press Ctrl+M to change NotEye modes, 'q' to win.");
  noteye_move(24, 15);
  noteye_addstr("F8 causes the NotEye script to crash.");

  while(true) {
    setTextAttr(8, 0);
    for(int y=2; y<22; y++)
    for(int x=0; x<80; x++) {
      noteye_move(y, x);
      noteye_addch('#');
      }

    for(int y=3; y<21; y++)
    for(int x=1; x<79; x++) {
      noteye_move(y, x);
      int dist = (x-pcx)*(x-pcx) + (y-pcy)*(y-pcy);
      if(dist > 200) dist = 200;
      // we use true color here
      setTextAttr32(0xFFFFFFF - 0x10101*dist, 0);
      noteye_addch('.');
      }
    
    noteye_move(pcy, pcx); setTextAttr(14, 0);
    noteye_addch('@');
    
    noteye_move(pcy, pcx); 
    
    int ch = noteye_getch();
    
    printf("ch = %d mod = %x\n", ch, noteye_getlastkeyevent()->key.keysym.mod);
    
    if(ch >= DBASE && ch < DBASE+8) {
      int d = ch & 7;
      pcx += dx[d];
      pcy += dy[d];
      // for simplicity we don't handle moving outside the area
      }
    
    if(ch == 'q') {
      printf("You have won!\n");
      break;
      }

    if(ch == NOTEYEERR) {

      // in some cases, noteye_getch() will return NOTEYEERR
      // after the script causes an error.
      // Restart NotEye is the only thing we can do then
      
      printf("NotEye error detected, restarting NotEye\n");

      noteye_halt();

      noteye_init();
      noteye_run("games/sample.noe", true);

      printf("restart OK\n");
      // we do not refresh the screen, so titles do not appear
      }
    }
  }

InternalProcess *IP;

void sampleError(int id, const char *b1, const char *b2, int param) {
  fprintf(stderr, "handling NotEye error #%d: %s", id, b1);
  if(b2) fprintf(stderr, " [%s]", b2);
  if(param != NOPARAM) fprintf(stderr, " [%d]", param);
  fprintf(stderr, "\n");
  }

int main(int argc, char **argv) {
  // the arguments are simply copied to the NotEye scripts
  noteye_args(argc, argv);  
  
  // initialize NotEye
  noteye_init();
  
  // handle errors
  noteye_handleerror(sampleError);

  // We could add additional Lua functions here. For example, Hydra Slayer
  // adds a function to give a complete information about the given cell
  // (more detailed than the ASCII character), and a function to give help
  // about the given cell (clicked with mouse). These functions are called
  // from the Lua script directly.

  noteye_run("games/sample.noe", true);

  // we follow the "threaded" approach. This means that UI runs in a
  // separate thread (Lua coroutine, actually). Hydra Slayer follows
  // another route -- the game is called from inside games/hydra.noe,
  // and the script works during getch().  
  rungame();  
  // tell the script that the game is finished
  noteye_finishinternal(1);
  // close the UI thread
  noteye_uifinish();
  // finish NotEye (this also exits the process)
  noteye_halt();
  }
