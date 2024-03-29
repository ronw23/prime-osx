// Necklace of the Eye v6.2
// roguelike frontend
// Copyright (C) 2010-2012 Zeno Rogue, see 'noteye.h' for details

static lua_State *LS;

// global arguments
static int gargc;
static char **gargv;

void noteye_args(int argc, char **argv) {
  gargc = argc; gargv = argv;
  }

// set Lua's global variables

void noteye_globalint(const char *name, int value) {
  lua_pushinteger(LS, value); lua_setglobal(LS, name);
  }

void noteye_globalstr(const char *name, const char *value) {
  lua_pushstring(LS, value); lua_setglobal(LS, name);
  }

void noteye_globalfun(const char *name, int f(lua_State *L)) {
  lua_register(LS, name, f);
  }

int lh_getevent(lua_State *L) {

  for(int i=0; i<size(eventobjs); i++) {
    if(objs[eventobjs[i]] && objs[eventobjs[i]]->checkEvent(L)) {
      return 1;
      }
    }
  SDL_Delay(10);
  lua_newtable(L);
  setfield(L, "type", 0);
  return 1;
  }

int lh_sleep(lua_State *L) {
  SDL_Delay(luaInt(1));
  return 0;
  }

int lh_colormix(lua_State *L) {
  checkArg(L, 2, "colormix");
  int a = luaInt(1);
  mixcolor(a, luaInt(2));
  return retInt(L, a);
  }

int lh_colormix3(lua_State *L) {
  checkArg(L, 3, "colormix3");
  int a = luaInt(1);
  mixcolorAt(a, luaInt(2), luaInt(3));
  return retInt(L, a);
  }

int lh_colorpart(lua_State *L) {
  checkArg(L, 2, "colorpart");
  int a = luaInt(1);
  return retInt(L, part(a, luaInt(2)));
  }

int lh_colorpartset(lua_State *L) {
  checkArg(L, 3, "colorset");
  int a = luaInt(1);
  part(a, luaInt(2)) = luaInt(3);
  return retInt(L, a);
  }

int lh_vgaget(lua_State *L) {
  checkArg(L, 1, "vgaget");
  return retInt(L, vgacol[luaInt(1)]);
  }

int lh_vgaset(lua_State *L) {
  checkArg(L, 2, "vgaset");
  vgacol[luaInt(1)] = luaInt(2);
  return 0;
  }

int lh_delete(lua_State *L) {
  checkArg(L, 1, "delete");
  int i = luaInt(1);
  if(i < 0 || i > size(objs)) {
    noteyeError(17, "delete: no such object", NULL, i);
    return 0;
    }
  deleteobj(i); // todo: reuse this index?
  return 0;
  }

int lh_objcount(lua_State *L) {
  return retInt(L, size(objs));
  }

int lh_logprint(lua_State *L) {
  logfile && fprintf(logfile, "%s", luaStr(1));
  return 0;
  }

int lh_logopen(lua_State *L) {
  checkArg(L, 1, "argv");
  logfile && logfile != stdin && fclose(logfile);
  const char *buf = luaStr(1);
  if(strcmp(buf, "-") == 0) logfile = stdout;
  else logfile = fopen(buf, "wt");
  errfile = logfile;
  return 0;
  }

int lh_bAND(lua_State *L) {
  return retInt(L, luaInt(1) & luaInt(2));
  }

int lh_bOR(lua_State *L) {
  return retInt(L, luaInt(1) | luaInt(2));
  }

int lh_bXOR(lua_State *L) {
  return retInt(L, luaInt(1) ^ luaInt(2));
  }

int lh_argv(lua_State *L) {
  checkArg(L, 1, "argv");
  int i = luaInt(1);
  if(i < 0 || i >= gargc) return 0;
  return retStr(L, gargv[i]);
  }

int lh_setfont(lua_State *L) {
  checkArg(L, 2, "setfont");
  Font *f = luaO(2, Font);
  Process *p = dluaO(1, Process);
  if(p) {
    p->f = f;
    }
  Screen *s= p ? p->s : dluaO(1, Screen);
  if(s) {
    for(int y=0; y<s->sx * s->sy; y++)
      s->v[y] = tileSetFont(s->v[y], f);
    }
  Tile *t = dluaO(1, Tile);
  if(t) return retInt(L, tileSetFont(t->id, f));
  return 0;
  }

char *noteyeStats() {
  static char buf[1000];
  sprintf(buf,
    "Objects allocated: %d\n"
    "Hashtable collisions: %d/%d\n"
    "Write compression: %d B -> %d B\n"
    "Read compression: %d B -> %d B\n"
    "Total size of images: %lld pixels (including %lld pixels in cache)\n",
    size(objs), hashcol, hashok, writeUnc, writeCmp, 
    readUnc, readCmp, totalimagesize, totalimagecache
    );

  return buf;
  }

int lh_noteyestats(lua_State *L) {
  return retStr(L, noteyeStats());
  }

int crashval = 0;

int lh_getcrashval(lua_State *L) {
  return retInt(L, crashval);
  }

int lh_setcrashval(lua_State *L) {
  checkArg(L, 1, "setcrashval");
  crashval = luaInt(1);
  return 0;
  }

int lh_geterrormsg(lua_State *L) {
  return retStr(L, noteyeerrbuf);
  return 0;
  }
   
void initLua() {
  LS = lua_open();
  
  if (!LS) {
    fprintf(errfile, "Error Initializing lua\n");
    exit(-1);
    }
 
  noteye_globalfun("loadimage", lh_loadimage);
  noteye_globalfun("newimage", lh_newimage);
  noteye_globalfun("imagetitle", lh_imagetitle);
  noteye_globalfun("fillimage", lh_fillimage);
  noteye_globalfun("saveimage", lh_saveimage);
  noteye_globalfun("imgcopy", lh_imgcopy);
  noteye_globalfun("imggetsize", lh_imggetsize);
  noteye_globalfun("getpixel", lh_getpixel);
  noteye_globalfun("setpixel", lh_setpixel);
  
  noteye_globalfun("addtile", lh_addTile);
  noteye_globalfun("tilemerge", lh_tileMerge);
  noteye_globalfun("tilemergeover", lh_tileMergeOver);
  noteye_globalfun("tilecol", lh_tileRecolor);
  noteye_globalfun("tilespatial", lh_tileSpatial);
  noteye_globalfun("tilelayer", lh_tileLayer);
  noteye_globalfun("tilexf", lh_tileTransform);
  noteye_globalfun("tilealpha", lh_tileAlpha);
  noteye_globalfun("tilefreeform", lh_tileFreeform);

  noteye_globalfun("tiledebug", lh_tiledebug);

  noteye_globalfun("freeformparam", lh_freeformparam);
  noteye_globalfun("freeformparamflags", lh_freeformparamflags);
  noteye_globalfun("getlayer", lh_getlayer);
  noteye_globalfun("getdistill", lh_getdistill);

  noteye_globalfun("gch", lh_gch);
  noteye_globalfun("gchv", lh_gchv);
  noteye_globalfun("gco", lh_gco);
  noteye_globalfun("gba", lh_gba);
  noteye_globalfun("gp2", lh_gp2);
  noteye_globalfun("gimg", lh_gimg);

  noteye_globalfun("gavcoba", lh_gavcoba);
  noteye_globalfun("tileavcobaf", lh_tileavcobaf);
  noteye_globalfun("getobjectinfo", lh_getobjectinfo);

  noteye_globalfun("newfont", lh_newfont);
  noteye_globalfun("fget", lh_getchar);
  noteye_globalfun("fgetav", lh_getcharav);

  noteye_globalfun("newscreen", lh_newScreen);
  noteye_globalfun("scrwrite", lh_scrwrite);
  noteye_globalfun("scrget", lh_scrget);
  noteye_globalfun("scrset", lh_scrset);
  noteye_globalfun("scrcopy", lh_scrcopy);
  noteye_globalfun("scrfill", lh_scrfill);
  noteye_globalfun("drawscreen", lh_drawScreen);
  noteye_globalfun("drawscreenx", lh_drawScreenX);
  noteye_globalfun("drawtile", lh_drawTile);
  noteye_globalfun("scrsave", lh_scrsave);
  noteye_globalfun("scrsetsize", lh_scrsetsize);
  noteye_globalfun("scrgetsize", lh_scrgetsize);

  noteye_globalfun("setvideomode", lh_setvideomode);
  noteye_globalfun("setvideomodei", lh_setvideomodex);
  noteye_globalfun("updaterect", lh_updaterect);
  noteye_globalfun("findvideomode", lh_findvideomode);
  noteye_globalfun("setwindowtitle", lh_setwindowtitle);
  noteye_globalfun("openconsole", lh_openconsole);
  noteye_globalfun("enablekeyrepeat", lh_enablekeyrepeat);
  
#ifndef INTERNALONLY
  noteye_globalfun("newprocess", lh_newProcess);
#endif

#ifdef LINUX
  noteye_globalfun("ansidebug", lh_ansidebug);
#endif

  noteye_globalfun("processactive", lh_processActive);
  noteye_globalfun("sendkey", lh_sendkey);
  noteye_globalfun("proccur", lh_proccur);
  noteye_globalfun("setfont", lh_setfont);

  noteye_globalfun("getevent", lh_getevent);
  noteye_globalfun("sleep", lh_sleep);
  noteye_globalfun("vgaget", lh_vgaget);
  noteye_globalfun("vgaset", lh_vgaset);
  noteye_globalfun("colormix", lh_colormix);
  noteye_globalfun("colormix3", lh_colormix3);
  noteye_globalfun("colorpart", lh_colorpart);
  noteye_globalfun("colorset", lh_colorpartset);
  noteye_globalfun("delete", lh_delete);
  noteye_globalfun("objcount", lh_objcount);
  noteye_globalfun("logprint", lh_logprint);
  noteye_globalfun("logopen", lh_logopen);

  noteye_globalfun("bAND", lh_bAND);
  noteye_globalfun("bOR", lh_bOR);
  noteye_globalfun("bXOR", lh_bXOR);
  noteye_globalfun("fpp", lh_fpp);
  noteye_globalfun("refreshconsole", lh_refreshconsole);

  noteye_globalfun("isoparam", lh_isoparam);
  noteye_globalfun("isosizes", lh_isosizes);
  noteye_globalfun("isoproject", lh_iso);

  noteye_globalfun("imagealias", lh_imagealias);
  
  noteye_globalfun("nwritefile", lh_writefile);
  noteye_globalfun("nreadfile", lh_readfile);
  noteye_globalfun("nwriteint", lh_writeint);
  noteye_globalfun("nreadint", lh_readint);
  noteye_globalfun("nwritestr", lh_writestr);
  noteye_globalfun("nreadstr", lh_readstr);
  noteye_globalfun("nwritescr", lh_writescr);
  noteye_globalfun("nreadscr", lh_readscr);
  noteye_globalfun("neof", lh_eof);
  noteye_globalfun("nflush", lh_flush);
  noteye_globalfun("nfinish", lh_finish);
  noteye_globalfun("nready", lh_ready);
  noteye_globalfun("nserver", lh_server);
  noteye_globalfun("naccept", lh_accept);
  noteye_globalfun("nconnect", lh_connect);

#ifndef NOAUDIO  
  noteye_globalfun("loadsound", lh_loadsound);
  noteye_globalfun("playsound", lh_playsound);
  noteye_globalfun("playsoundloop", lh_playsoundloop);
  noteye_globalfun("mixsetdistance", lh_mixsetdistance);
  noteye_globalfun("mixsetpanning", lh_mixsetpanning);
  noteye_globalfun("mixunregisteralleffects", lh_mixunregisteralleffects);

  noteye_globalfun("loadmusic", lh_loadmusic);
  noteye_globalfun("playmusic", lh_playmusic);
  noteye_globalfun("playmusicloop", lh_playmusicloop);
  noteye_globalfun("musicon", lh_musicon);
  noteye_globalfun("musicvolume", lh_musicvolume);
  noteye_globalfun("musichalt", lh_musichalt);
  noteye_globalfun("fadeoutmusic", lh_fadeoutmusic);

#endif

  noteye_globalfun("uicreate", lh_uicreate);
  noteye_globalfun("uisleep", lh_uisleep);

  noteye_globalfun("noteyestats", lh_noteyestats);

  noteye_globalfun("setcrashval", lh_setcrashval);
  noteye_globalfun("getcrashval", lh_getcrashval);
  noteye_globalfun("geterrormsg", lh_geterrormsg);


  // constants:
  
  #ifdef LINUX
  noteye_globalint("linux", 1);
  #else
  noteye_globalint("windows", 1);
  #endif
  
  #ifdef OPENGL
  noteye_globalint("opengl", 1);
  #endif

  noteye_globalfun("internal", lh_internal);
  noteye_globalint("network", NETWORK);

  noteye_globalint("Gfx", 1);
  
  noteye_globalint("transAlpha", transAlpha);
  noteye_globalint("transNone", transNone);

  noteye_globalint("spFlat",  spFlat);
  noteye_globalint("spFloor", spFloor);
  noteye_globalint("spCeil",  spCeil);
  noteye_globalint("spMonst", spMonst);
  noteye_globalint("spItem",  spItem);
  noteye_globalint("spWallN", spWallN);
  noteye_globalint("spWallE", spWallE);
  noteye_globalint("spWallS", spWallS);
  noteye_globalint("spWallW", spWallW);
  noteye_globalint("spIFloor",spIFloor);
  noteye_globalint("spICeil", spICeil);
  noteye_globalint("spIItem", spIItem);
  noteye_globalint("spIWallL",spIWallL);
  noteye_globalint("spIWallR",spIWallR);
  noteye_globalint("spFree"  ,spFree);
  
  noteye_globalint("evKeyDown", evKeyDown);
  noteye_globalint("evKeyUp", evKeyUp);
  noteye_globalint("evProcScreen", evProcScreen);
  noteye_globalint("evProcQuit", evProcQuit);
  noteye_globalint("evMouseMotion", evMouseMotion);
  noteye_globalint("evMouseDown", evMouseDown);
  noteye_globalint("evMouseUp", evMouseUp);
  noteye_globalint("evBell", evBell);
  noteye_globalint("evQuit", evQuit);
  
  noteye_globalstr("noteyeversion", NOTEYEVERSION);
  noteye_globalint("NOTEYEVER", NOTEYEVER);
  noteye_globalint("nepScreen", nepScreen);
  noteye_globalint("nepWait",   nepWait);
  noteye_globalint("nepKey",    nepKey);
  noteye_globalint("nepFace",   nepFace);
  noteye_globalint("nepMode",   nepMode);
  noteye_globalint("nepMessage",nepMessage);
  noteye_globalint("nepCursor", nepCursor);
  noteye_globalint("nepMouse",  nepMouse);

  noteye_globalint("recDefault",  recDefault);
  noteye_globalint("recMult",     recMult);
  noteye_globalint("recPurple",   recPurple);

  noteye_globalint("argc", gargc);
  noteye_globalfun("argv", lh_argv);
    
  luaL_openlibs(LS);   
  }

void closeLua() {
  lua_close(LS);
  }

void lua_stackdump (lua_State *L) {
      printf("dif: %p %p\n", L, LS);
      int i;
      int top = lua_gettop(L);
      printf("top = %d\n", top);
      for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {
    
          case LUA_TSTRING:  /* strings */
            printf("`%s'", lua_tostring(L, i));
            break;
    
          case LUA_TBOOLEAN:  /* booleans */
            printf(lua_toboolean(L, i) ? "true" : "false");
            break;
    
          case LUA_TNUMBER:  /* numbers */
            printf("%g", lua_tonumber(L, i));
            break;
    
          default:  /* other values */
            printf("%s", lua_typename(L, t));
            break;
    
        }
        printf("  ");  /* put a separator */
      }
      printf("\n");  /* end the listing */
    }
