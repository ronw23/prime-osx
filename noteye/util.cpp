// Necklace of the Eye v6.2
// roguelike frontend
// Copyright (C) 2010-2011 Zeno Rogue, see 'noteye.h' for details

static vector<int> eventobjs;

static vector<Object*> objs;
static const char *lastfn;

extern noteyehandler noteyeErrorHandler;

static void noteyeError(int id, const char *b1, const char *b2, int param = NOPARAM);

// critical errors
static FILE *errfile;

// log file
static FILE *logfile;

Object* noteye_getobj(int id) {
  if(id <= 0 || id > (int) objs.size()) {
    noteyeError(20, "no such object", lastfn, id);
    return NULL;
    }
  return objs[id];
  }
  
Object* noteye_getobjd(int id) {
  if(id <= 0 || id > (int) objs.size()) return NULL;
  return objs[id];
  }

void noteye_wrongclass(int id, lua_State *L) {
#ifdef USELUA
  if(L) {
    static char buf[256];
    sprintf(buf, "object %d of wrong class in %s", id, lastfn);
    lua_pushstring(L, buf);
    lua_error(L);
    }
#endif
  noteyeError(2, "object of wrong class", lastfn, id);
  }

void deleteobj(int idx) {
  delete objs[idx];
  objs[idx] = NULL;
  }

static int ZZ;

int& qpixel(SDL_Surface *surf, int x, int y) {
  if(x<0 || y<0 || x >= surf->w || y >= surf->h) return ZZ;
  char *p = (char*) surf->pixels;
  p += y * surf->pitch;
  int *pi = (int*) (p);
  return pi[x];
  }

// should probably be changed for big endian architectures...
uint8& part(int& col, int i) {
  uint8* c = (uint8*) &col;
  return c[i];
  }

void recolor(int& col, int ncol, int mode) {

  if(mode == recMult) {
    for(int p=0; p<4; p++)
      part(col, p) = (part(col, p) * part(ncol, p) + 127) / 255;
    return;
    }

  if(mode == recPurple)
    if(!(part(col,0) == part(col,2) && part(col,0) > part(col,1)))
      return;

  uint8 cmax = 0, cmin = 255;
  for(int p=0; p<3; p++)
    cmax = max(cmax, part(col,p)), 
    cmin = min(cmin, part(col,p));
  for(int p=0; p<3; p++)
    part(col,p) = cmin + ((1+cmax-cmin) * part(ncol, p))/ 256;
  }

void mixcolor(int& col, int ncol) {
  col = ((col&0xFEFEFE) + (ncol&0xFEFEFE)) >> 1;
  }

uint8 mixpart(uint8 a, uint8 b, uint8 c) {
  return (a*(255-c)+b*c+127)/255;
  }

bool istrans(int a, int b) { return (a&0xFFFFFF) == (b&0xFFFFFF); }

void mixcolorAt(int& col, int ncol, int p) {
  for(int i=0; i<4; i++)
    part(col,i) = mixpart(part(col,i), part(ncol,i), part(p, i));
  }

typedef unsigned char uint8;

void alphablend(int& col, int ncol) {
  int s3 = part(ncol, 3);
  for(int i=0; i<3; i++) {
    part(col, i) = 
      (part(col, i) * (255-s3) + part(ncol, i) * s3 + 128) >> 8;
    }
  part(col, 3) = s3;
  }

void alphablendc(int& col, int ncol, bool cache) {
  if(cache) col = ncol;
  else alphablend(col, ncol);
  }

template<class T> int size(T& x) { return x.size(); }

SDL_Surface* newImage(int x, int y) {
  SDL_Surface *surf = SDL_CreateRGBSurface(SDL_HWSURFACE, x, y, 32, 0,0,0,0);
  if(!surf)
    noteyeError(3, "failed to create an image", NULL);
  return surf;
  }

int sti(string s) { return atoi(s.c_str()); }
int sth(string s) { int ret; sscanf(s.c_str(), "%x", &ret); return ret; }

int tty_fix();

int vgacol[16] = {
  0x0000000, 0x10000aa, 0x200aa00, 0x300aaaa, 0x4aa0000, 0x5aa00aa, 0x6aa5500, 0x7aaaaaa,
  0x8555555, 0x95555ff, 0xA55ff55, 0xB55ffff, 0xCff5555, 0xDff55ff, 0xEffff55, 0xFffffff
  };

#define Get(T, name, id) \
  T* name = dbyId<T> (id)

// #include <typeinfo>

int registerObject(Object *o) {
  int res = objs.size();
  // printf("O:%d: %p [%s]\n", res, o, typeid(*o).name());
  objs.push_back(o);
  return o->id = res;
  }

#ifdef USELUA
// lua utils

int retInt(lua_State *L, int i) {
  lua_pushinteger(L, i);
  return 1;
  }

int retBool(lua_State *L, bool b) {
  lua_pushboolean(L, b);
  return 1;
  }

int retStr(lua_State *L, string s) {
  lua_pushstring(L, s.c_str());
  return 1;
  }

int retStr(lua_State *L, char *s) {
  lua_pushstring(L, s);
  return 1;
  }

int retObject(lua_State *L, Object *o) {
  return retInt(L, registerObject(o));
  }

int retObjectEv(lua_State *L, Object *o) {
  int id = registerObject(o);
  eventobjs.push_back(id);
  return retInt(L, id);
  }

void checkArg(lua_State *L, int q, const char *fname) {
  lastfn = fname;
  if(lua_gettop(L) != q) {
    noteyeError(4, "bad number of arguments", fname, q);
    }
  }

void setfield (lua_State *L, const char *index, int value) {
  lua_pushstring(L, index);
  lua_pushinteger(L, value);
  lua_settable(L, -3);
}

void setfieldStr (lua_State *L, const char *index, const char *str) {
  lua_pushstring(L, index);
  lua_pushstring(L, str);
  lua_settable(L, -3);
}

void setfieldBool (lua_State *L, const char *index, bool b) {
  lua_pushstring(L, index);
  lua_pushboolean(L, b);
  lua_settable(L, -3);
}

void noteye_lua_newtable(lua_State *L) { lua_newtable(L); }

bool havefield(lua_State *L, const char *key) {
  bool result;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  result = lua_isnil(L, -1);
  lua_pop(L, 1);
  return result;
  }

int getfield (lua_State *L, const char *key) {
  int result;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  result = luaInt(-1);
  lua_pop(L, 1);
  return result;
  }

long double getfieldnum (lua_State *L, const char *key) {
  long double result;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  result = luaNum(-1);
  lua_pop(L, 1);
  return result;
  }

int getfield (lua_State *L, const char *key, int def) {
  int result;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  result = lua_isnil(L, -1) ? def : luaInt(-1);
  lua_pop(L, 1);
  return result;
  }

long double getfieldnum (lua_State *L, const char *key, long double def) {
  long double result;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  result = lua_isnil(L, -1) ? def : luaNum(-1);
  lua_pop(L, 1);
  return result;
  }


int noteye_int(lua_State *L, int x) {
  return (int) luaL_checkint(L, x);
  }

long double noteye_num(lua_State *L, int x) {
  return luaL_checknumber(L, x);
  }

bool noteye_bool(lua_State *L, int x) {
  return (bool) lua_toboolean(L, x);
  }

const char *noteye_str(lua_State *L, int x) {
  return luaL_checkstring(L, x);
  }

#endif

static char noteyeerrbuf[512];

void noteyeError(int id, const char *b1, const char *b2, int param) {
  if(b2 && param != NOPARAM)
    sprintf(noteyeerrbuf, "%s [%s] %d", b1, b2, param);
  else if(b2)
    sprintf(noteyeerrbuf, "%s [%s]", b1, b2);
  else if(param != NOPARAM)
    sprintf(noteyeerrbuf, "%s [%d]", b1, param);
  else 
    sprintf(noteyeerrbuf, "%s", b1);
  (*noteyeErrorHandler) (id, b1, b2, param);
  }

void noteyeError2(int id, const char *b1, const char *b2, int param) {
  fprintf(errfile, "NotEye error #%d: %s", id, b1);
  if(b2) fprintf(errfile, " [%s]", b2);
  if(param != NOPARAM) fprintf(errfile, " [%d]", param);
  fprintf(errfile, "\n");
  noteye_halt();
  exit(1);
  }

noteyehandler noteyeErrorHandler = &noteyeError2;
