-- Necklace of the Eye v6.5 roguelike frontend
-- Copyright (C) 2010-2013 Zeno Rogue, see COPYING for details

-- Various functions, related to keyboard input somehow

-- directions and keys for them

-- direction 0 is right, and then we go anti-clockwise

dofile(commondir.."sdlkeys.lua")
numpad = {256+9, 256+8, 256+7, 256+4, 256+1, 256+2, 256+3} numpad[0] = 256+6
numberkeys = {"9", "8", "7", "4", "1", "2", "3"} numberkeys[0] = "6"

arrowkeys = {SDLK_PAGEUP, SDLK_UP, SDLK_HOME, SDLK_LEFT, SDLK_END, SDLK_DOWN, SDLK_PAGEDOWN}
arrowkeys[0] = SDLK_RIGHT
arrowkeys.ktype = 4
vikeytab = {"u", "k", "y", "h", "b", "j", "n"}
vikeytab[0] = "l"
vikeytab.ktype = 8
pseudovikeytab = {"i", "u", "y", "h", "b", "n", "m"}
pseudovikeytab[0] = "j"
pseudovikeytab.ktype = 8
leftkeytab = {"e", "w", "q", "a", "z", "x", "c"}
leftkeytab[0] = "d"
leftkeytab.ktype = 8
wasdkeytab = {"u", "w", "y", "a", "b", "s", "n"}
wasdkeytab[0] = "d"
wasdkeytab.ktype = 4

function fixchr(ev)
  if ev.symbol >= 1 and ev.symbol < 128 then
    ev.chr = ev.symbol
  else
    ev.chr = 0
    end
  end

function assignkey(ev, ch)
  if type(ch) == "number" then
    ev.symbol = ch
    ev.chr = 0
  else
    ev.symbol = av(ch)
    if ev.type == evKeyDown then
      ev.chr = av(ch)
    else
      ev.chr = 0
      end
    end
  end

function checkkey(ev, ch)
  if type(ch) == "number" then
    return ev.symbol == ch
  else
    return ev.chr == av(ch)
    end
  end

-- DIRECTION KEY PREPROCESSING 
--=============================

-- facing, for the FPP display

facedir = 0

-- change face by 'd'
-- usually changed in two steps:
-- on keydown (type==evKeyDown) and keyup (type==evKeyUp)
-- unless onlydown is specified
function changeface(d, type, onlydown)
  ofd = facedir
  if gamefourdir then d = 2*d end
  if gamehexdir and ((ofd+d) % 4 ==2) then 
    d = 2*d
    end
  
  if type == evKeyDown and onlydown then
    facedir = ofd + d
    drawandrefresh()
       
  elseif type == evKeyDown then
    facedir = ofd + 0.5 * d
    drawandrefresh()
    facedir = ofd + d
  else
    -- facedir = math.floor((ofd + 0.5 * d) % 8 + .001)
    drawandrefresh()
    setfacedir(facedir)
    end
  end
                                          
-- modifier key used for diagonal movement
-- (usually depends on config, returns 0 if this config option is off)

function upmodifier()
  return (diagleft and KMOD_LSHIFT or 0) + (diagright and KMOD_RSHIFT or 0)
  end

function downmodifier()
  return (diagleft and KMOD_LCTRL or 0) + (diagright and KMOD_RCTRL or 0)
  end

-- is the modifier key (e.g., KMOD_LALT for the left ALT) active? 
function modon(ev, x)
  return bAND(ev.mod, x) > 0 
  end

-- should we add up/down to a cursor because Shift/Ctrl has been pressed,
-- and the respective config option is on?

function modup(ev)
  return modon(ev, upmodifier())
  end

function moddn(ev)
  return modon(ev, downmodifier())
  end

-- left/right keys + shift/ctrl = diagonal movement

function modupdown(ev)
  if ev.symbol == SDLK_RIGHT and modup(ev) then
    ev.symbol = SDLK_PAGEUP
    ev.mod = 0
    end
  if ev.symbol == SDLK_RIGHT and moddn(ev) then
    ev.symbol = SDLK_PAGEDOWN
    ev.mod = 0
    end
  if ev.symbol == SDLK_LEFT and modup(ev) then
    ev.symbol = SDLK_HOME
    ev.mod = 0
    end
  if ev.symbol == SDLK_LEFT and moddn(ev) then
    ev.symbol = SDLK_END
    ev.mod = 0
    end
  end

-- should we rotate keys entered?

function dorotate(ev)
  return mapon
  end

-- adjust the direction key entered to the current perspective  
function tryrotate(dirmap, ev)
  if not dirmap then return end
  for x=0,7 do 
    if checkkey(ev, dirmap[x]) then
    
      local od = type(dirmap[x]) ~= "number"
    
      if mode == modeTiles or mode == modeASCII or mode == modeMini or mode == modeMed or not mapon then
        setfacedir(x)
        assignkey(ev, dirkeys[x])
        return 1
      elseif mode == modeISO then
        assignkey(ev, dirkeys[(1+x)%8])
        return 1
      elseif x == 4 and dirmap.ktype == 4 then changeface(1, ev.type, od) return 2
      elseif x == 0 and dirmap.ktype == 4 then changeface(-1, ev.type, od) return 2
      elseif x == 1 then changeface(-1, ev.type, od) return 2
      elseif x == 3 then changeface( 1, ev.type, od) return 2
      elseif x == 5 then changeface( 2, ev.type, od) return 2
      elseif x == 7 then changeface(-2, ev.type, od) return 2
      else 
        local idx = math.floor((facedir+6+x)%8)
        assignkey(ev, dirkeys[idx])
        return 1
        end
      end
    end
  return nil
  end

-- preprocess and send the direction key to the game/server
function sendrotated(ev)
  modupdown(ev)
  if dorotate(ev) then
    local v = tryrotate(currentKeymap.map,ev) or 
      (rotatenumpad and tryrotate(numpad,ev)) or 
      (rotatearrowkeys and tryrotate(arrowkeys,ev))
    if not v and currentKeymap.extra then v = currentKeymap.extra(ev) end
    if v == 2 then return end
    end
  nsendkey(ev)
  end

function switchKL(ev)
  if checkkey(ev, "k") then assignkey(ev, "m") end
  if checkkey(ev, "l") then assignkey(ev, "i") end
  return 0
  end

function switchLeft(ev)
  if checkkey(ev, "y") then assignkey(ev, "q") end
  if checkkey(ev, "u") then assignkey(ev, "e") end
  if checkkey(ev, "b") then assignkey(ev, "z") end
  if checkkey(ev, "n") then assignkey(ev, "c") end
  if checkkey(ev, "h") then assignkey(ev, "a") end
  if checkkey(ev, "j") then assignkey(ev, "x") end
  if checkkey(ev, "k") then assignkey(ev, "w") end
  if checkkey(ev, "l") then assignkey(ev, "d") end
  return 0
  end

function switchWASD(ev)
  if checkkey(ev, "h") then assignkey(ev, "a") end
  if checkkey(ev, "j") then assignkey(ev, "s") end
  if checkkey(ev, "k") then assignkey(ev, "w") end
  if checkkey(ev, "l") then assignkey(ev, "d") end
  return 0
  end

dirkeys = numpad

mapNone = {name = "none"}
mapVI = {name = "YUHJKLBN (VI keys)", map = vikeytab}
mapPseudoVI = {name = "YUIHJBNM", map = pseudovikeytab}
mapPseudoVIplus = {name = "YUIHJBNM + remap KL", map = pseudovikeytab, extra = switchKL}
mapWASD = {name = "WASD", map = wasdkeytab}
mapWASDplus = {name = "WASD + remaps", map = wasdkeytab, extra = switchWASD}
mapQWEADZXC = {name = "QWEADZXC", map = leftkeytab}
mapQWEADZXCplus = {name = "QWEADZXC + remaps", map = leftkeytabm, extra = switchLeft}

gotKeymaps = {mapNone, mapVI, mapPseudoVI, mapPseudoVIplus, mapWASD, mapWASDplus, mapQWEADZXC, mapQWEADZXCplus}
currentKeymap = initialKeymap or mapNone
lastKeymap = nil

havemusic = true
havebrogue = true

useuserremaps = gotuserremaps and not remapsoff

cphase = 0

menuF4 = true
menuBracket = true
menuCtrlM = true

function ismenukey(ev)
  if menuF4 and ev.type == evKeyDown and ev.symbol == SDLK_F4 then
    return true
  elseif menuBracket and ev.type == evKeyDown and ev.chr == 29 then
  elseif menuCtrlM and ev.type == evKeyDown and ev.symbol ~= 13 and ev.symbol ~= SDLK_KP_ENTER and ev.chr == 13 then
    return true
    end
  return false
  end

function isfullscreenkey(ev)
  return ev.type == evKeyDown and ev.symbol == SDLK_RETURN and modon(ev, KMOD_LALT + KMOD_RALT)
  end

-- user configurable keymapping

if not userkeyremaps then
  userkeyremaps = {}
  end

function remapkey(_orig, _new, _cond)
  gotuserremaps = true
  useuserremaps = not remapsoff
  table.insert(userkeyremaps, {orig = _orig, new = _new, cond = _cond})
  end

function ingame(G) return function(ev) return gamename == G and not inmenu end end
function inallgames(ev) return not inmenu end
function inallmenus(ev) return inmenu end
function onlymenu(M) return function(ev) return inmenu == M end end
function onlymap(c) return function(ev) return mapon and c(ev) end end

function userremap(ev)
  if not useuserremaps then return end
  for k,v in pairs(userkeyremaps) do
    if checkkey(ev, v.orig) and v.cond(ev) then
      assignkey(ev, v.new)
      return
      end
    end
  end

-- is the key a modifier key?
function modkey(sym)
  return 
    sym == SDLK_LSHIFT or sym == SDLK_LCTRL or
    sym == SDLK_RSHIFT or sym == SDLK_RCTRL or
    sym == SDLK_LALT or sym == SDLK_RALT  
  end

-- == keys menu ==

keysmenu = {}

function namediag()
  if diagleft and diagright then
    return "both shift/ctrl"
  elseif diagleft then
    return "left shift/ctrl"
  elseif diagright then
    return "right shift/ctrl"
  else
    return "off"
    end
  end

addtomenu(keysmenu, "k", writechoicef(function() return "letters as directions: "..currentKeymap.name end),
  function ()
    if lastkeymap and not keymapidx then
      currentKeymap,lastkeymap = lastkeymap, currentKeymap
      keymapidx = (currentKeymap == gotKeymaps[1]) and 1 or 0
    else
      if not lastkeymap then lastkeymap = currentKeymap end
      if not keymapidx then 
        keymapidx = (currentKeymap == gotKeymaps[1]) and 1 or 0
        end
      keymapidx = keymapidx + 1
      if keymapidx > #gotKeymaps then keymapidx = 1 end
      currentKeymap = gotKeymaps[keymapidx]
      end
    return true
    end
  )

addtomenu(keysmenu, "r", writechoicef(function() return "user keymap (config.lua): "..boolonoff(useuserremaps) end),
  function()
    useuserremaps = not useuserremaps
    return 1
    end,
  function() return not gotuserremaps end
  )

addtomenu(keysmenu, "n", writechoicef(function() return "rotate/translate numpad: "..boolonoff(rotatenumpad) end),
  function ()
    rotatenumpad = not rotatenumpad
    return true
    end
  )
  
addtomenu(keysmenu, "a", writechoicef(function() return "rotate/translate arrowkeys: "..boolonoff(rotatearrowkeys) end),
  function ()
    rotatearrowkeys = not rotatearrowkeys
    return true
    end
  )
  
addtomenu(keysmenu, "d", writechoicef(function() return "diagonal movement with arrow keys: "..namediag() end),
  function ()
    diagleft = not diagleft
    if diagleft then diagright = not diagright end
    return true
    end
  )

addtomenu(mainmenu, "k", writechoice("configure keyboard input"),
  function()
    menuexecute(keysmenu, dfthdr(
      "NotEye is able to preprocess the keyboard input for you.\n\n"..
      "When playing in the Isometric or FPP mode, you probably want the direction\n"..
      "keys to act intuitively. NotEye rotates them for you.\n\n"..
      "Many modern keyboards lack the numpad, and thus lack an easy way to move\n"..
      "diagonally in roguelikes. NotEye can solve that by allowing diagonal\n"..
      "movement with Shift/Ctrl + Left/Right arrow, or by using a letter layout\n"..
      "(such as QWEADZXC, YUIHJBNM, or the VI keys).\n\n"..
      "NotEye will output directions in the way expected by the roguelike.\n"..
      "For example, if a roguelike understands only the VI keys, then arrow\n"..
      "keys, numpad, and/or letter layout will be translated to VI keys.\n\n"..
      "Configure input:"
      ))
    end
  )

