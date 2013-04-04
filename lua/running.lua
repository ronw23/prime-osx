-- Necklace of the Eye v6.5 roguelike frontend
-- Copyright (C) 2010-2013 Zeno Rogue, see COPYING for details
-- contains functions for running the game

-- the main loop

if not justrefreshing then
  function noteyesleep()
    sleep(1)
    end
  end

function mainloopcyc()
  inmenu = nil
  local ev = getevent()
  
  if ismenukey(ev) then
    ev.type = evKeyUp
    ev.symbol = SDLK_LCTRL
    nsendkey(ev)
    noteyemenu()
    drawandrefresh()
  elseif ev.type == evKeyDown and ev.symbol == SDLK_F2 and quickshots then
    local fname = shotdir..curdate()..".bmp"
    saveimage(1, fname)
  elseif isfullscreenkey(ev) then
    fscr = not fscr
    setvideomodex()
    drawandrefresh()
  elseif ev.type == evKeyDown or ev.type == evKeyUp then
    userremap(ev)
    sendrotated(ev)
  elseif ev.type == evMouseDown or ev.type == evMouseUp or ev.type == evMouseMotion then
    mousepos = ev
    sendmouse(ev)
  elseif ev.type == evBell then
    if not SndBell then
      SndBell  = loadsound(sounddir.."rune.ogg")
      end
    playsound(SndBell, volsound)
  elseif ev.type == evQuit then
    if VIEW then
      delete(VIEW)
      VIEW = nil
      end
    if SERVER then
      delete(SERVER)
      SERVER = nil
      end
    sendquit()
  elseif ev.type == evProcQuit then
    EXIT = ev.exitcode
    stoploop = true
    return 0
  elseif ev.type == evProcScreen then  
    broadcast(sendscreen)
    broadcast(nflush)
    setcursor(proccur(P))
    copymap()
    drawandrefresh()
  elseif VIEW and viewpause <= 0 then
    if neof(VIEW) then
      delete(VIEW)
      VIEW = nil
      localmessage("recording finished")
    else
      readdata(VIEW, true, true)
      end
  elseif checkstreams() then
  elseif ev.type == 0 then
    copymap()
    drawandrefresh()
    checkmusic()
    if REC then recpause = recpause + recspeed end
    if VIEW then viewpause = viewpause - viewspeed end
    noteyesleep()
    end
  return 1
  end

function mainloop()
  while mainloopcyc() > 0 and not stoploop do
    end
  end

function sendmouse(ev)
  local d
  
  if ev.type == evMouseMotion then
    lastmm = ev
    return
    end
  
  if ev.type == evMouseUp then
    return
    end

  d = pixeltodir(ev)
  
  if ev.button == 4 and d ~= -1 then
    d = (d+4) % 8
    end
  
  if d ~= -1 then
    ev.type = evKeyDown
    ev.mod = 0
    assignkey(ev, dirkeys[d])
    sendrotated(ev)
  else
    ev.type = evKeyDown
    ev.symbol = av(".")
    ev.chr = av(".")
    ev.mod = 0
    nsendkey(ev)
    end
  end

-- == start the game process ==

function caller(name, dir, uname, wname)
  if manualcall and linux then
    return "sh -l"
  elseif manualcall then
    return "cmd"
  elseif linux then
    return "sh "..noteyedir.."caller.sh "..dir.." "..uname.." \""..name.."\""
  else
    return ".\\caller.bat "..dir.." "..wname.." \""..name.."\""
    end
  end

function caller3(name, x)
  return caller(name, x, x, x)
  end

function rungamex(cmdline)
  P = newprocess(S, Font, cmdline)
  end

function rungame2(cmdline)

  prepareconsole()
  stoploop = false
  pcursor = {x=0, y=0}
  
  if autorec then startrecording() end
  if autoserve then startserver() end
  
  if not viewmode then
    rungamex(cmdline)
    end

  mainloop()

  if REC then delete(REC); REC = nil end
  if VIEW then delete(VIEW); VIEW = nil end
  if ACCEPT then stopserver() end
  if SERVER then delete(SERVER); SERVER = nil end
  if havemain then createmain() end
  if P then delete(P); P = nil end
  end

function rungame(cmdline)

  if justrefreshing then
    return
    end
  
  if threaded and not viewmode then

    local lnoteyesleep = noteyesleep
    
    function noteyesleep()
      uisleep()
      lnoteyesleep()
      end
  
    function rungamex(cmdline)

      P = internal(S, Font, cmdline)
      end
    
    function rg()
      rungame2(cmdline)
      end
  
    uicreate(rg)
  
  else
    rungame2(cmdline)
    end
  end

function sendquit()
  -- for libtcod as libnoteye user (not officially released)
  if vialibtcod then
    quitlibtcod()
    end
  end

