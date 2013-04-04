-- Necklace of the Eye v6.5 roguelike frontend
-- Copyright (C) 2010-2013 Zeno Rogue, see COPYING for details

-- the menu subsystem

-- == menu subsystem ==

menuy = 1

function addtomenu(M, key, writer, act, hid)
  table.insert(M, {key = key, displayAt = writer, action = act, hidden = hid})
  end

function writechoice(x)
  return function(M) 
    scrwrite(IMG, 1, menuy, string.upper(M.key).." - "..x, Font, vgaget(15))
    menuy = menuy + 1
    end
  end

function writechoicef(x)
  return function(M) 
    scrwrite(IMG, 1, menuy, string.upper(M.key).." - "..x(), Font, vgaget(15))
    menuy = menuy + 1
    end
  end

function writechoicecol(x, cf)
  return function(M) 
    scrwrite(IMG, 1, menuy, M.key.." - "..x, Font, vgaget(cf()))
    menuy = menuy + 1
    end
  end

function dfthdr(title)
  return function()
    menuy = 1
    titl = title
    while string.find(titl, "\n") do
      local i = string.find(titl, "\n")
      scrwrite(IMG, 1, menuy, string.sub(titl, 1, i-1), Font, vgaget(14))
      titl = string.sub(titl, i+1)
      menuy = menuy+1
      end
    scrwrite(IMG, 1, menuy, titl, Font, vgaget(14))
    menuy = menuy + 2
    end
  end

function mainhdr()
  menuy = 6
  title = 
    hydraonly and "Hydra Slayer "..hydraver
    or "Necklace of the Eye "..noteyeversion.." - "..gamename
  scrwrite(IMG, 1, 1, title, Font, vgaget(14))
    
  scrwrite(IMG, 1, 3, gameon and "press ENTER to resume playing" or "press ENTER to return to the main menu", Font, vgaget(7))
  scrwrite(IMG, 1, 4, "NotEye website: http://roguetemple.com/z/noteye.php", Font, vgaget(7))
  end

if noteyelogo == nil then
  noteyelogo = loadimage(gfxdir.."noteye-logo-dark.png")
  noteyetile = addtile(noteyelogo, 0, 0, 400, 300, -1)
  end

-- view the menu (stored in IMG) on the screen
function viewmenu()
  if havegfx then
    drawtile(1, noteyetile, 0, 0, xscrsize.x, xscrsize.y)
    
    scrcopy(IMG, 0,0, IMGX, 0,0, conssize.x, conssize.y, cellblacknb)
                                   
    drawscreen(1, IMGX,-1, 0, fontsize.x, fontsize.y)
    drawscreen(1, IMGX, 0,-1, fontsize.x, fontsize.y)
    drawscreen(1, IMGX, 1, 0, fontsize.x, fontsize.y)
    drawscreen(1, IMGX, 0, 1, fontsize.x, fontsize.y)

    drawscreen(1, IMG, 0, 0, fontsize.x, fontsize.y)
    drawmessage()
    updscr()
    end
  if haveascii then
    scrcopy(IMG, 0, 0, mainscreen, 0,0, conssize.x, conssize.y, eq)
    drawmessage()
    refreshconsole()
    end
  end

function menuexecute(M, hdr, tail)
  while true do
    scrfill(IMG, 0, 0, conssize.x, conssize.y, 0)

    hdr()
    -- 
    
    for k,v in pairs(M) do
      if not (v.hidden and v.hidden()) then
        v.displayAt(v)
        end
      end
    
    if tail then tail() end

    viewmenu()
    inmenu = M

    lev = getevent()
    cont = false

    if lev.type == evMouseDown then
      local y = math.floor(lev.y / fontsize.y)
      local x = math.floor(lev.x / fontsize.x)
      
      local ch = gch(scrget(IMG, 1, y))
      lev.type = evKeyDown
      lev.symbol = av(string.lower(ch))
      lev.chr = av(string.lower(ch))
      end

    if lev.type == evKeyDown then
      userremap(lev)
      local chr = string.char(lev.chr)
      for k,v in pairs(M) do
        if chr == v.key and not (v.hidden and v.hidden()) then
          cont = v.action(v)
          break
          end
        end
      if not cont then break end
    elseif checkstreams() then
    elseif lev.type == 0 then
      sleep(10)
      end
    end
  clrscr()
  end

-- == ask for string, and applymod ==

function askstr(text, question, warning)
  while true do
    scrfill(IMG, 0, 0, conssize.x, conssize.y, 0)

    local function write(y, text)
      scrwrite(IMG, 1, y, text, Font, vgaget(15))
      end

    write(1, question)
    
    write(3, text)
    
    write(5, "ENTER to submit, ESC to cancel, TAB to erase")
    
    if warning then
      write(7, "Warning: BE CAREFUL WHILE RUNNING AS A SERVER!")
      write(8, "see the website for details: ")
      write(9, "http://www.roguetemple.com/z/noteye.php")
      end
    
    viewmenu()

    inmenu = "askstr"
    lev = getevent()
    if lev.type == evKeyDown then
      userremap(lev)
      if lev.symbol == 27 then 
        return
      elseif lev.symbol == SDLK_TAB then 
        text = ""
      elseif lev.symbol == SDLK_RETURN then 
        return text
      elseif lev.symbol == SDLK_BACKSPACE then 
        text = string.sub(text, 0, #text-1)
      elseif lev.chr > 0 then
        text = text .. string.char(lev.chr)
        end
    elseif checkstreams() then
    elseif lev.type == 0 then
      sleep(10)
      end
    end

  end

function applymod(val, mod)
  if bAND(mod, KMOD_LSHIFT + KMOD_RSHIFT) > 0 then
    return val / 2
  elseif bAND(mod, KMOD_LCTRL + KMOD_RCTRL) > 0 then
    local ret = askstr(""..val, "Enter a new value:")
    if ret and tonumber(ret) then return tonumber(ret)
    else return val end
  else
    return val * 2
    end
  end

-- == screenshots menu ==
function gameonly() return gamename == "auto" end
function nogameonly() return gamename ~= "auto" end

shotsmenu = {}

addtomenu(shotsmenu, "b", writechoice("BMP"), function()
    copymap()
    drawdisplay()
    local fname = shotdir..curdate()..".bmp"
    saveimage(1, fname)
    end
  )

addtomenu(shotsmenu, "h", writechoice("HTML"), function()
    scrsave(S, shotdir..curdate()..".html", 0)
    end
  )

addtomenu(shotsmenu, "p", writechoice("phpBB"), function()
    scrsave(S, shotdir..curdate()..".txt", 1)
    end
  )

-- == tech menu ==

techmenu = {}

addtomenu(techmenu, "s", writechoicef(function()
  return "S - measure fps" end),
  function()
    fps_start = os.clock()
    frames = 0
    end
  )

addtomenu(techmenu, "r", writechoice("refresh NotEye script files"),
  function()
    print "Refreshing configuration."
    justrefreshing = true
    dofile(commondir.."generic.lua")
    prepareconsole()
    if gamename ~= "auto" then dofile(gamesdir..gamename..".lua") end
    prepareconsole()
    justrefreshing = false
    end
  )

addtomenu(techmenu, "t", writechoice("tile test"),
  function()
    dofile(commondir.."tiletest.lua")
    end
  )

addtomenu(techmenu, "c", writechoice("console helper"),
  function()
    conshelp = not conshelp
    end
  )

-- == main menu ==

mainmenu = {}

-- == ok, done ==

function noteyemenu()
  keymapidx = nil
  
  if frames then
    localmessage("frames per second = "..frames/frametime)
    frames = nil
    fps_start = nil
    end
    
  menuexecute(mainmenu, mainhdr)
  end

-- these menus should be added to the bottom of the main menu,
-- so we call moremenus() after including all files which put
-- something in the menu

function moremenus()
  addtomenu(mainmenu, "g", writechoice("grab a screenshot"),
    function()
      menuexecute(shotsmenu, dfthdr("Select the screenshot format:"))
      end,
    gameonly
    )
  
  addtomenu(mainmenu, "d", writechoice("NotEye development functions"),
    function()
      menuexecute(techmenu, dfthdr("NotEye stats:\n\n"..noteyestats().."\nDevelopment functions:"))
      end
    )
  
  addtomenu(mainmenu, "c", writechoice("view using a selected config"),
    function()
      viewmode = true
      end,
    nogameonly
    )
  
  addtomenu(mainmenu, "q", writechoice("stop viewing mode"),
    function()
      stoploop = true
      end,
    function() return not viewmode end
    )
  
  -- this is executed after adding everything (sounds, keys etc) to the menu
  if usermenus then usermenus() end
  end
