// Necklace of the Eye v6.6
// roguelike frontend
// Copyright (C) 2010-2012 Zeno Rogue, see 'noteye.h' for details

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

// includes

#define NOTEYEVERSION "6.7"
#define NOTEYEVER 0x500

#include <stdio.h>
#include <algorithm>
#include <unistd.h>
#include <math.h>

#include <complex>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <vector>

// define SDLFLAT if SDL files do not use a subdirectory

#ifdef SDLFLAT
#include <SDL.h>
#include <SDL_image.h>
#undef main
#elif defined(MAC)
#include <SDL/SDL.h>
#include <SDL_image/SDL_image.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#undef main
#endif

#ifdef OPENGL
#ifdef MAC
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#endif

struct lua_State;

using namespace std;

// types

typedef unsigned char uint8;

// basic object type

// Objects can be affected by Lua script

struct Object {
  int id;
  virtual bool checkEvent(struct lua_State *L) {return false;}
  virtual ~Object() {}
  };

int registerObject(Object* o);

// image

struct Image : Object {
  SDL_Surface *s;
  bool locked;
  int changes;
  void setLock(bool lock);
  Image(const char *fname);
  Image(int sx, int sy, int color = 0);
  Image();
  ~Image();
  string title;
  };

// tile

struct Tile : Object {
  virtual void preprocess() const {}
  Tile *nextinhash, **previnhash;
  Tile() : nextinhash(0), previnhash(0) {}
  ~Tile();
  virtual void debug() { printf("Tile\n"); }
  };

void tts(int id);

struct TileImage : Tile {
  Image *i;
  int ox, oy, sx, sy, trans;
  int chid; // character ID
  struct GLtexture *gltexture;
  int fppdown; // push down pixels, for FPP
  vector<struct TransCache*> caches;
  virtual int hash() const { return 0; };
  virtual void debug();
  TileImage(int _sx, int _sy);
  ~TileImage();
  };

int getFppDown(TileImage *T);

struct TileMerge : Tile {
  int t1, t2;
  bool over;
  virtual int hash() const;
  virtual void debug();
  };

struct TileRecolor : Tile {
  int t1, color, mode;
  virtual int hash() const;
  int cache, cachechg;
  virtual void preprocess();
  virtual void debug();
  ~TileRecolor();
  };

struct TileSpatial : Tile {
  int t1, sf;
  virtual int hash() const;
  virtual void debug();
  };

struct TileLayer : Tile {
  int t1, layerid;
  virtual int hash() const;
  virtual void debug();
  };

struct TileTransform : Tile {
  int t1;
  double dx, dy, sx, sy, dz, rot;
  virtual int hash() const;
  virtual void debug();
  };

struct FreeFormParam : Object {
  double d[4][4];
  // 0 = draw both sides
  // 1 = draw normal side only
  // 2 = draw back side only
  // 3 = reverse sides
  // 4 = do not change
  int side;
  // use 'shiftdown' on the resulting image
  bool shiftdown;
  };

// 3D point, used in the FPP mode

struct fpoint4 {
  double x, y, z;
  fpoint4() {}
  fpoint4(double X, double Y, double Z) {x=X; y=Y; z=Z;}
};

// viewpar struct for the FPP

struct viewpar {
  int x0, x1, y0, y1, xm, ym, xs, ys;
  double xz, yz;
  int ctrsize, monsize, objsize, monpush, objpush;
  bool shiftdown;
  int side;
  fpoint4 delta;
  };


struct drawmatrix {
  int x, y, tx, ty, txy, tyx, tzx, tzy;
  };

struct TileFreeform : Tile {
  int t1;
  FreeFormParam *par;
  virtual int hash() const;
  virtual void debug();
  };

struct TileFill : Tile {
  int color, alpha;
  virtual int hash() const;
  virtual void debug();
  TileImage *cache;
  };

// font

struct Font : Object {
  int *ti;
  int cnt;
  ~Font() { delete ti; }
  };

// screen

struct Screen : Object {
  int sx, sy;
  vector<int> v;
  void setSize(int _sx, int _sy);
  void write(int x, int y, const char *buf, Font *f, int color);
  int& get(int x, int y);
  };

// graphics

struct GFX : Image {
  int flags;
  virtual bool checkEvent(struct lua_State *L);
  };

// process

struct Process : Object {
  Screen *s;
  Font *f;
  const char *cmdline;
  bool isActive;
  int exitcode;
  bool active() { return isActive; }
  Process(Screen *scr, Font *f, const char *cmdline) : s(scr), f(f), cmdline(cmdline) {}
  int curx, cury;

  virtual void sendKey(int symbol, int mod, bool down, int unicode) = 0;
  };

// internal process

#define IBUFSIZE 16
#define EBUFSIZE 16

struct InternalProcess : Process {

  InternalProcess(Screen *scr, Font *f, const char *cmdline);
  ~InternalProcess();
  
  int back, fore, brushback, brush0;
  
  // int kbuf[IBUFSIZE], modbuf[IBUFSIZE], kbs, kbe, lastmod;
  
  union SDL_Event *evbuf[EBUFSIZE], *lastevent;
  int evs, eve;

  void setColor(int fore, int back);

  void lf();
  void drawChar(int c);
  int gp(int x, int dft);
  
  void applyM(int i);

  bool checkEvent(lua_State *L);
  void sendKey(int symbol, int mod, bool down, int unicode);
  
  bool changed;
  int  exitcode;
  };

// push tool

template <class T> struct push {
  T& x, saved;
  push(T& y, T val) : x(y) {saved=x; x=val;}
  ~push() {x = saved;}
  };

// streams:

#include <zlib.h>
#define BUFSIZE 4096

struct NStream : Object {
  set<int> knownout;       // for output streams
  map<int, int> knownin;   // for input  streams

  // primitives
  virtual void writeCharPrim(char c) = 0;
  virtual char readCharPrim() = 0;
  virtual bool eofPrim() = 0;
  virtual bool readyPrim() = 0;

  void flush();
  void writeChar(char c);
  char readChar();
  bool eof();
  void proceed(bool check);
  bool ready();
  
  void writeInt(int v);
  void writeDouble(double x);
  int readInt();
  double readDouble();

  void writeStr(const string& s) {
    int sz = s.size();
    writeInt(sz);
    for(int i=0; i<sz; i++) writeChar(s[i]);
    }
  string readStr() {
    int size = readInt();
    string v;
    v.resize(size);
    for(int i=0; i<size; i++) v[i] = readChar();
    return v;
    }
  void writeObj(int x);
  int readObj();

  void readScr(Screen *s);
  void writeScr(Screen *s);
  
  Bytef outbuf_in[BUFSIZE];
  Bytef outbuf_out[BUFSIZE];
  Bytef inbuf_in[BUFSIZE];
  Bytef inbuf_out[BUFSIZE];
  z_stream zout, zin;
  bool outok, inok, finished;
  int cblock;
  NStream() : outok(false), inok(false), finished(false) {}
  void finish() { finished = true; }
  };

struct NOFStream : NStream {
  FILE *f;
  void writeCharPrim(char c) { fwrite(&c, 1, 1, f); }
  char readCharPrim() { printf("write-only\n"); exit(1); }
  ~NOFStream();
  bool eofPrim() { return feof(f); }
  bool readyPrim() { return true; }
  };

struct NIFStream : NStream {
  FILE *f;
  char readCharPrim() { char c; if(fread(&c, 1, 1, f)) ; return c; }
  void writeCharPrim(char c) { printf("read-only\n"); exit(1); }
  ~NIFStream() { if(f) fclose(f); }
  bool eofPrim() { return feof(f); }
  bool readyPrim() { return true; }
  };

NStream *openTCPStream(void *skt);

// console output

struct MainScreen : public Screen {
  ~MainScreen();
  MainScreen();
  bool checkEvent(lua_State *L);
  };

// sound

#ifndef NOAUDIO
#ifdef SDLFLAT
#include <SDL_mixer.h>
#elif defined(MAC)
#include <SDL_mixer/SDL_mixer.h>
#else
#include <SDL/SDL_mixer.h>
#endif
#else
typedef struct Mix_Chunk Mix_Chunk;
typedef struct Mix_Music Mix_Music;
#endif

struct Sound : Object {
  Mix_Chunk *chunk;
  ~Sound();
  int play(int vol, int loops = 0);
  };

struct Music : Object {
  Mix_Music *chunk;
  ~Music();
  void play(int loops);
  };

// consts:

// use the alpha channel
#define transAlpha (-0xABED)

// no transparent color (actually, we simply hope that this color does not appear)
#define transNone  (-0xABEEAD)

// spatial flags

#define spFlat     1
#define spFloor    2
#define spCeil     4
#define spMonst    8
#define spItem     16
#define spCenter   32
#define spIFloor   64
#define spIItem    128

#define spICeil    0x00200
#define spIWallL   0x00400
#define spIWallR   0x00800
#define spWallN    0x01000
#define spWallE    0x02000
#define spWallS    0x04000
#define spWallW    0x08000
#define spFree     0x10000

// events

#define evKeyDown     1
#define evKeyUp       2
#define evProcScreen  3
#define evProcQuit    4
#define evMouseMotion 5
#define evMouseDown   6
#define evMouseUp     7
#define evBell        8
#define evQuit        9

// NotEye protocol

#define nepScreen     1
#define nepWait       2
#define nepKey        3
#define nepFace       4
#define nepMode       5
#define nepMessage    6
#define nepCursor     7
#define nepMouse      8

// recoloring algorithms
#define recDefault    0
#define recMult       1
#define recPurple     2

// libnoteye

void noteye_args(int argc, char **argv);
void noteye_init();
void noteye_run(const char *noemain, bool applyenv);
void noteye_halt();

#define NOPARAM (-10000)

typedef void (*noteyehandler)(int id, const char *b1, const char *b2, int param);
void noteye_handleerror(noteyehandler h);

// Lua utils

void checkArg(lua_State *L, int q, const char *fname);

#define luaInt(x) noteye_int(L, x)
#define luaNum(x) noteye_num(L, x)
#define luaBool(x) noteye_bool(L, x)
#define luaStr(x) noteye_str(L, x)

#define luaO(x,t) (byId<t> (luaInt(x), L))
#define dluaO(x,t) (dbyId<t> (luaInt(x)))

int noteye_int(lua_State *L, int);
long double noteye_num(lua_State *L, int);
bool noteye_bool(lua_State *L, int);
const char *noteye_str(lua_State *L, int);

int retInt(lua_State *L, int i);
int retBool(lua_State *L, bool b);
int retStr(lua_State *L, char *s);

void noteye_globalint(const char *name, int value);
void noteye_globalstr(const char *name, const char *value);
void noteye_globalfun(const char *name, int f(lua_State *L));

Object *noteye_getobj(int id);
Object *noteye_getobjd(int id);
void noteye_wrongclass(int id, lua_State *L);

template<class T> T* byId(int id, lua_State *L) {
  T* g = dynamic_cast<T*> (noteye_getobj(id));
  if(!g) noteye_wrongclass(id, L);
  return g;
  }

template<class T> T* dbyId(int id) {
  return dynamic_cast<T*> (noteye_getobjd(id));
  }

// make Curses calls refer to Prc, and call the object at stack position 'spos'
// on ghch

InternalProcess *noteye_getinternal();
void noteye_setinternal(InternalProcess *Proc, lua_State *L, int spos);
void noteye_finishinternal(int exitcode);

#define CALLUI "noteye_callui"

void lua_stackdump(lua_State *L);
void noteye_uiresume();
void noteye_uifinish();

Font *newFont(Image *base, int inx, int iny, int trans);
void noteye_initnet();

int addMerge(int t1, int t2, bool over);
int addRecolor(int t1, int color, int mode);
int addFill(int color, int alpha);
