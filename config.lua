-- NotEye configuration file

-- default visual settings:
----------------------------

-- start in fullscreen mode
fscr = false

-- use OpenGL initially?
useGL = false

-- which mode to start in (modeASCII, modeTiles, modeFPP, modeISO, modeMini)
mode = modeTiles -- modeISO

-- modepower == 10 : always use the mode above
-- modepower ==  0 : always start in the default mode for the given game
-- intermediate values : depending on the game
-- (e.g. DoomRL switches to FPP if initmodepower<7)
modepower = 0

-- default font
-- curfont = "cp437-16"
curfont = "dejavu-20"
curfonttype = fontSimpleA
fontsize = {rx=32, ry=8}
fontscale = {x=1, y=1}

-- reserve lines for messages below the screen
msgreserve = 0

-- have graphics
havegfx = true

-- have ascii (Note: in ASCII, NotEye menu is ran with Ctrl+[, not Ctrl+M)
-- also, translating TrueColor games back to 16 colors does not work well yet
haveascii = false

-- input settings
------------------

-- allow to use SHIFT/CTRL for diagonal movement
diagleft = true
diagright = true

-- rotate both numpad and arrow keys
rotatenumpad = true
rotatearrowkeys = true

-- see generic.noe for other names
-- initialKeymap = mapWASDplus

-- rec/view/server/client configuration
-----------------------------------------

-- initial recording and viewing speed

recspeed = 1000
viewspeed = 1000

-- default filenames

recfile = "record.nev"
viewfile = "view.nev"

-- binary flags: enter "false" or "true"
-- automatically start recording
autorec = false
-- start in view mode instead of game mode
viewmode = false
-- start server automatically (make sure that you know what you are doing!)
autoserve = false
-- record mode changes by default?
servemode = true
-- accept input from clients by default?
serveinput = true
-- should we get font from the stream?
takefont = false

-- default network configuration
--------------------------------

-- for client:
portclient = 6677
serverhost = "localhost"

-- libtcod client:
libtcodhost = "localhost"
libtcodport = 6678

-- for server:
portserver = 6677

-- audio options
----------------

volsound = 100
volmusic = 33

-- use the default music even if the game's noe file
-- disables it, or defines its own soundtrack
forcemusic = false

-- user-defined music:
-- resetmusic()
-- addmusic("sound/music1.ogg")
-- addmusic("sound/music2.ogg")

-- misc options
---------------

-- allow taking screenshots quickly with F2
quickshots = false

-- automatically connect to the libtcod game, or to server
-- (also can be activated with -tcod and -server command line options)
-- autoconnecttcod = true
-- autoconnectserver = true

-- enable this to see the coordinates and RGB colors of characters in libtcod on mouseover
conshelp = false

-- do you want to force external Hydra Slayer
externalhydra = false

-- change this to true if you want to start the games yourself
-- (useful e.g. for playing via a telnet/ssh server)
manualcall = false

-- user key remapping
---------------------

-- user key remappings: some examples

-- uncomment if you want them to be disabled by default
-- remapsoff = true

-- replace "1" and "2" in Hydra Slayer, only while viewing the map
-- remapkey("1", "2", onlymap(ingame("hydra")))
-- remapkey("2", "1", onlymap(ingame("hydra")))

-- replace F1 and F2 in Hydra Slayer (see sdlkeys.noe for other key names)
-- remapkey(SDLK_F1, SDLK_F2, ingame("hydra"))
-- remapkey(SDLK_F2, SDLK_F1, ingame("hydra"))

-- replace "z" and "y" in all games
--remapkey("z", "y", inallgames)
--remapkey("y", "z", inallgames)

-- make Enter start Hydra Slayer in the main menu
--remapkey(SDLK_RETURN, "h", inmenu("noteyemain"))

-- user menus: allow you to quickly select some options
-- see menu.noe to see how to add more options

function usermenus()
-- addgraphmode("1", "quick ASCII", modeASCII, mainmenu)
-- addFont("2", "Fantasy font 16x32", "fantasy-32", 32, 8, fontSimple, mainmenu)
-- addtomenu(mainmenu, "3", writechoice("big tiles"), function() tilesize={x=64,y=64} end)
  end
