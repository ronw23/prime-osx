-- Necklace of the Eye v6.5 roguelike frontend
-- Copyright (C) 2010-2013 Zeno Rogue, see COPYING for details

-- contains various generic functions for reading the process' map

function xminimap(C)
  b = 0
  local ch = gch(C)
  local av = av(ch)
  
  if ch == " " then b = 0
  elseif ispc(C) then b = 0xFFFFFF
  elseif ch == "." then b = 0x8404040
  elseif av == 250     then b = 0x8404040
  elseif av == 249     then b = 0x8404040
  elseif ch == "#" then b = 0xC0C0C0
  elseif av == 176     then b = 0xC0C0C0
  elseif av == 177     then b = 0xC0C0C0
  elseif av == 178     then b = 0xC0C0C0
  elseif (ch >= "A" and ch <= "Z") or (ch >= "a" and ch <= "z") or (ch == "@") then b = 0x2F08000
  elseif ch == (">") or ch == ("<") then b = 0xFF00FF
  else b = 0x0000FF
  end
  
  return tilefill(b)
  end

function ispc(C)
  return gch(C) == "@"
  end

function inmapregion(point)
  return point.x > mapregion.x0 and point.x < mapregion.x1 and point.y > mapregion.y0 and point.y < mapregion.y1
  end

function pcat(where)
  return where and inmapregion(where) and ispc(scrget(S, where.x, where.y))
  end

function findpc()
  -- maybe player is under the cursor
  if pcat(pcursor) then 
    ncursor = pcursor

  -- if not, look for him...
  else 
    for x=mapregion.x0,mapregion.x1-1 do
    for y=mapregion.y0,mapregion.y1-1 do
      if ispc(scrget(S, x,y)) then
        ncursor = {x=x, y=y}
        end
      end end
    end

  mapon = pcat(ncursor)
  end

function blinkcolor()
  return 0xF000000 + 0x10101 * (128 + math.floor(math.sin(cphase/3) * 100))
  end
  
function addcursor(C)
  local Cur=tilerecolor(fget(Font, "_"), blinkcolor())
  return tilemergeover(C, Cur)
  -- return tilemerge(tilerecolor(fget(Font, "_"), 0xF000000 + 0x10101 * (128 + math.floor(math.sin(cphase/3) * 100))), C)
  end

function copymap()
  local ncs = scrgetsize(S)
  if ncs.x ~= conssize.x or ncs.y ~= conssize.y then
    conssize = ncs
    prepareconsole()
    end

  -- copy from the process map
  scrcopy(S, 0, 0, IMG, 0,0, conssize.x, conssize.y, eq)

  cphase = cphase + 1
  cursor = pcursor
  scrset(IMG, cursor.x, cursor.y, addcursor(scrget(IMG, cursor.x, cursor.y)))

  findpc()
  
  if mapon then
    -- yeah, there he is!
    playerpos = {x=ncursor.x-mapregion.x0, y=ncursor.y-mapregion.y0}
    scrset(IMG, 999, 999, 0) -- 0 outside the map
    scrcopy(S, mapregion.x0, mapregion.y0, MAP, 0,0, mapsize.x, mapsize.y, eq)

    if mode ~= modeASCII then 
      scrfill(IMG, mapregion.x0, mapregion.y0, mapsize.x, mapsize.y, 0)
      end
    
    readgmessages()
  else
    mapon = false
    end
  end

-- read the process' lines

function readline(Scr, x1, x2, y)
  s = ""
  for x = x1,x2-1 do 
    s = s .. gch(scrget(Scr, x, y))
    end
  return s
  end

function getline(y)
  return readline(S, 0, conssize.x-1, y)
  end

-- we got a message from the game
-- (override to do special effects, e.g., sound effects)
function gotgmessage(s)
  end

-- read the game's messages
function readgmessages()
  if msgbox == nil then return true end
  local oldgmsg = gmsg
  local function testeqmsg(oldofs, count, offset)
    for y = oldofs,count do 
      if gmsg[y] ~= oldgmsg[y+offset] then 
        return false 
        end
      end
    return true
    end
  gmsg = {}
  msglines = msgbox.y1-msgbox.y0
  for y = 1,msglines do 
    gmsg[y] = readline(S, msgbox.x0, msgbox.x1, msgbox.y0-1+y)
    end
  if not oldgmsg then
    for y = 1,msglines do 
      gotgmessage(gmsg[y])
      end
    return
    end
  for nmsg = 0,msglines do
    if testeqmsg(1, msglines-nmsg, nmsg) then
      for y = 1,nmsg do 
        gotgmessage(gmsg[msglines-nmsg+y])
        end
      return
      end
    end
  end

