-- Necklace of the Eye v5.2 roguelike frontend
-- Copyright (C) 2010-2012 Zeno Rogue, see COPYING for details
-- the main script

noteyeloaded = true

if not noteyedir then
  noteyedir = (os.getenv("NOTEYEDIR") or ".").."/"
  end

gamesdir = noteyedir .. "lua/"
commondir = noteyedir .. "lua/"
gfxdir = noteyedir .. "gfx/"
sounddir = noteyedir .. "sound/"
shotdir = "shot/"

noeflags = not hydraver
i = 1
configfile = configfile or "config.lua"

while argv(i) do
  if argv(i) == "-N" then noeflags = true
  elseif argv(i) == "-N0" then noeflags = false
  elseif noeflags and argv(i) == "-C" then 
    i = i+1
    configfile = argv(i)
  elseif noeflags and argv(i) == "-tcod" then 
    autoconnecttcod = true
  elseif noeflags and argv(i) == "-connect" then 
    autoconnectserver = true
  elseif noeflags and argv(i) == "-L" then 
    i = i+1
    logopen(argv(i))
  elseif noeflags and argv(i) == "-X" then 
    i = i+1
    game_to_launch = argv(i)
  elseif noeflags and argv(i) == "--ascii" then 
    setascii = true
    end
  i = i+1
  end

havemain = true

print("Necklace of the Eye v"..noteyeversion.." (C) 2010-2012 Zeno Rogue")

dofile(commondir.."generic.lua")

function createmain()
  dofile(commondir.."generic.lua")
  prepareconsole()
  createmainscr()
  end

function createmainscr()
  
  setwindowtitle("Necklace of the Eye v"..noteyeversion)
  scrfill(IMG, 0, 0, 80, 25, 0)
  
  local Col = vgaget(10)

  local function write(y, text)
    scrwrite(IMG, 1, y, text, Font, Col)
    end
  
  local function writef(y, text)
    scrwrite(IMG, 40, y, text, Font, Col)
    end
  
  local function writex(x, y, text)
    scrwrite(IMG, x, y, text, Font, Col)
    end
  
  write(1, "Welcome to Necklace of the Eye v"..noteyeversion.."!")

  Col = vgaget(7)
  write(2, "Copyright (C) 2011-2012 Zeno Rogue")
  write(3, "Homepage: http://www.roguetemple.com/z/noteye.php")
  write(4, "NotEye is distributed under GNU General Public License version 2")
  write(5, "It comes with absolutely no warranty; see the file COPYING for details")

  Col = vgaget(15)
  write(13, "Choose the game to play...")
  
  Col = vgaget(14)
  if hydraver and not externalhydra then
    write(15, "H - Hydra Slayer "..hydraver.." (integrated into NotEye)")
  else
    write(15, "H - Hydra Slayer")
    end

  write(17, "R - Rogue         L - nLarn")
  write(18, "D - DoomRL        P - PRIME")
  write(19, "A - ADOM          C - Crawl")
  write(20, "F - Frozen Depths N - NetHack")

  Col = vgaget(7)
  writef(17, "check the website to see how well")
  writef(18, "these games are supported currently")

  Col = vgaget(15)
  write(22, "G - generic roguelike (shell)")
  write(23, "Q - quit")
  
  Col = vgaget(7)
  write( 7, "Necklace of the Eye (NotEye for short) is a frontend for playing roguelikes ")
  write( 8, "and other console/libtcod programs. It enhances them by providing tiles, FPP ")
  write( 9, "view, mouse support, scripting, taking HTML/phpBB screenshots, fullscreen, ")
  write(10, "recording and viewing videos, live streaming, sound, and possibly many more. ")
  write(11, "Press Ctrl+M, Ctrl+[ or F4 while playing to get NotEye's menu.")
  
  if viewmode then
    Col = vgaget(9)
    writex(41, 22, "select your configuration")
  else 
    Col = vgaget(12)
    writex(41, 22, "M - libtcod and more options")
    writex(41, 23, "(Brogue, Drakefire Chasm, topDog etc)")
    end
  end

function startbyname(name)
  gamename = name
  local savemode = mode
  local savemodepower = modepower
  if name == "generic" then 
    setmode(1)
    dofile(commondir.."generic-tile.lua")
    setwindowtitle("NotEye: generic roguelike")
    local cmdline = linux and ("sh -l") or "cmd.exe"
    P = rungame(cmdline)
  elseif string.match(name, "^[a-zA-Z0-9]*$") then
    dofile(gamesdir..name..".lua")
  else
    print("Unknown name: "..name)
    end
  if not threaded then 
    mode = savemode
    modepower = savemodepower
    end
  end

function defaultmodep(x, prio)
  -- other modes simply hide a part of the display with no graphics
  if modepower >= 10 then -- do nothing
  elseif not havegfx then mode = modeASCII
  elseif crashsafe then mode = modeASCII
  elseif modepower < prio then mode = x
  end
  end

function defaultmode(x)
  defaultmodep(x, 5)
  end

-- run some tests
function starttest()
  local img = newimage(64,64, 0)
  for y=0,63 do for x=0,63 do
    setpixel(img,x,y,(x+y)%2 == 1 and 0xFFFFFFFF or 0xFF000000)
    end end
  local til = addtile(img, 0, 0, 64, 64, transAlpha)
  while true do
  
    rot = freeformparam(
      1,0.5,0,0,
      0,0.5,0.5,0,
      0,-0.5,0.5,0,
      0,0,0,0
      )

    drawtile(1, noteyetile, 0, 0, xscrsize.x, xscrsize.y)
    drawtile(1, til, 0, 0, 64, 64)
    drawtile(1, til, 64, 0, 32, 64)
    drawtile(1, til, 96, 0, 32, 32)
    drawtile(1, tilefreeform(til, rot), 0, 64, 64,64)
    updscr()
    lev = getevent()
    if lev.type == evKeyDown then break end
    end
  end

function noteyemain()

  if noteyecrash then 
    dofile(commondir.."crashmenu.lua")
    return
    end
  
  if game_to_launch then
    havemain = false
    prepareconsole()
    startbyname(game_to_launch)
    return
    end

  createmain()
  while true do
    createmainscr()
    if autoconnecttcod then
      connecttotcod()
      autoconnecttcod = false
      end
    if autoconnectserver then
      connecttoserver()
      autoconnectserver = false
      end
    viewmenu()
    inmenu="noteyemain"
    lev = getevent()
    
    if lev.type == evQuit then break
    elseif lev.type == evMouseDown then
      local y = math.floor(lev.y / 16)
      local ch = gch(scrget(IMG, 1, y))
      lev.type = evKeyDown
      lev.symbol = av(string.lower(ch))
      end
      
    if lev.type == evMouseMotion then

    elseif lev.type == evKeyDown then
      userremap(lev)
    
      if lev.chr == av("q") then break
      elseif lev.chr == av("h") then startbyname("hydra")
      elseif lev.chr == av("f") then startbyname("frozen")
      elseif lev.chr == av("a") then startbyname("adom")
      elseif lev.chr == av("c") then startbyname("crawl")
      elseif lev.chr == av("p") then startbyname("prime")
      elseif lev.chr == av("d") then startbyname("doomrl")
      elseif lev.chr == av("r") then startbyname("rogue")
      elseif lev.chr == av("l") then startbyname("nlarn")
      elseif lev.chr == av("n") then startbyname("nethack")
      elseif lev.chr == av("g") then startbyname("generic")
      elseif lev.chr == av("z") then starttest()
      elseif lev.chr == av("m") or ismenukey(lev) then 
        gamename = "auto"
        noteyemenu()
      elseif isfullscreenkey(lev) then
        fscr = not fscr
        setvideomodex()
        end

    else
      sleep(10)
      end
    end
  end

dofile (configfile)
clonetrack(soundtrack, baksoundtrack)
selectfont(scalefont(loadfont(curfont, fontsize.rx, fontsize.ry, curfonttype), fontscale.x, fontscale.y))

mode = mode or modeASCII

if setascii then
  havegfx = false
  haveascii = true
  end

--function mainscript()
--  profstart("total")
--  noteyemain()
--  profend("total")
--  profinfo()
--  end

-- noteyemain()

noteyemain()
