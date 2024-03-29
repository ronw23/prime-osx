-- Necklace of the Eye v5.1 roguelike frontend
-- Copyright (C) 2010-2012 Zeno Rogue, see COPYING for details

-- we use the following globals to keep connected streams:

-- REC: we record to this stream
-- VIEW: we view from this stream
-- ACCEPT: server we are running
-- SERVER: server we are connecting to
-- CLIENTS: the list of clients connected to us

function sendscreen(X)
  nwriteint(X, nepScreen)
  nwritescr(X, S)
  end

function sendcursor(X)
  nwriteint(X, nepCursor)
  nwriteint(X, pcursor.x)
  nwriteint(X, pcursor.y)
  end

function sendfacedir(X)
  nwriteint(X, nepFace)
  nwriteint(X, facedir)
  end

function sendmode(X)
  if servemode then
    nwriteint(X, nepMode)
    nwriteint(X, mode)
    end
  end

if not CLIENTS then
  CLIENTS = {}
  end

-- broadcast to REC 

function broadcast(F)
  if REC then
    recsync()
    F(REC)
    end
  for x=1,table.getn(CLIENTS) do
    F(CLIENTS[x])
    end
  end

function setcursor(pos)
  if pcursor ~= pos then
    pcursor = pos
    broadcast(sendcursor)
    broadcast(nflush)
    end
  end

function setfacedir(dir)
  facedir = dir
  broadcast(sendfacedir)
  broadcast(nflush)
  end

function setmode(nmode)
  mode = nmode
  broadcast(sendmode)
  broadcast(nflush)
  end

function sendmessage(X)
  nwriteint(X, nepMessage)
  nwritestr(X, message)
  end
  
function addmessage(str)
  localmessage(str)
  broadcast(sendmessage)
  broadcast(nflush)
  end

function checkclients()
  if ACCEPT then
    local CLIENT = naccept(ACCEPT)
    if CLIENT > 0 then
      initrec(CLIENT)
      sendscreen(CLIENT)
      sendfacedir(CLIENT)
      sendcursor(CLIENT)
      sendmode(CLIENT)
      nflush(CLIENT)
      table.insert(CLIENTS, CLIENT)
      localmessage("client #"..CLIENT.." connected")
      return true
      end
    end
  for x=1,table.getn(CLIENTS) do
    local CLIENT = CLIENTS[x]
    if nready(CLIENT) then
      if neof(CLIENT) then
        localmessage("client #"..CLIENT.." disconnected")
        delete(CLIENT)
        table.remove(CLIENTS, x)
        return true
      else
        readdata(CLIENT, false, false)
        return true
        end
      end
    end
  return false
  end

scrcount = 0
function readdata(X, rscr, rtime)
  scrcount = scrcount + 1
  nep = nreadint(X)
  if nep == nepMessage then
    addmessage(nreadstr(X))
  elseif nep == nepScreen and rscr then
    nreadscr(X, S, onimagenotfound)
    if StreamSetFont == 1 then
      I = gimg(scrget(S,1,1))
      if I > 0 then
        for k,v in pairs(Fonts) do
          if v.png == I then
            selectfontfull(k)
            end
          end
        I = 0
        end
      end
    if StreamSetFont == 2 then
      setfont(S, Font)
      end
    copymap()
    broadcast(sendscreen)
    broadcast(nflush)
  elseif nep == nepCursor and rscr then
    setcursor({x = nreadint(X), y = nreadint(X)})
  elseif nep == nepFace and rscr then
    setfacedir(nreadint(X))
  elseif nep == nepMode and rscr then
    setmode(nreadint(X))
  elseif nep == nepWait and rtime then
    viewpause = viewpause + nreadint(X)
  elseif nep == nepKey then
    ev = {}
    ev.symbol = nreadint(X)
    ev.mod = nreadint(X)
    ev.type = nreadint(X)
    if (not StreamVer) or StreamVer >= 0x500 then
      ev.chr = nreadint(X)
    else
      ev.chr = 0
      end
    if serveinput then
      nsendkey(ev)
      end
  elseif nep == nepMouse then
    ev = {}
    ev.x = nreadint(X)
    ev.y = nreadint(X)
    ev.state = nreadint(X)
    if serveinput then
      netsendmouse(ev)
      end
  elseif neof(X) then
  else
    nreadint(-1)
    end
  end
       
function startserver()
  ACCEPT = nserver(portserver)
  end

function stopserver()
  delete(ACCEPT)
  ACCEPT = nil
  for x=1,table.getn(CLIENTS) do
    delete(CLIENTS[x])
    end
  CLIENTS = {}
  end
  
function recsync()
  if recpause > 0 then
    nwriteint(REC, nepWait)
    nwriteint(REC, recpause)
    end
  recpause = 0
  end

function startviewing()
  VIEW = nreadfile(viewfile)
  if VIEW == 0 then
    VIEW = nil
    localmessage("could not open file "..viewfile)
    print("error!")
    return
    end
  viewpause = 0
  initviewer(VIEW)
  end

function startrecording()
  REC = nwritefile(recfile)
  if REC == 0 then
    REC = nil
    localmessage("could not open file "..recfile)
    return
    end
  initrec(REC)
  recpause = 0
  end

function initrec(X)
  nwritestr(X, "NotEye stream")
  nwriteint(X, NOTEYEVER)
  nwritestr(X, gamename)
  end

function initviewer(X)
  StreamType = nreadstr(X)
  StreamVer = nreadint(X)
  StreamName = nreadstr(X)
  logprint("stream = "..StreamType.." ver = "..StreamVer.." game = "..StreamName.."\n")
  StreamSetFont = takefont and 1 or 2
  
  if gamename == "auto" then
    local lvm = viewmode
    viewmode = true
    startbyname(StreamName)
    viewmode = lvm
    gamename = "auto"
    end
  end

function nsendkey(ev)
  if P then sendkey(P, ev) end
  if SERVER then
    nwriteint(SERVER, nepKey)
    nwriteint(SERVER, ev.symbol)
    nwriteint(SERVER, ev.mod)
    nwriteint(SERVER, ev.type)
    nwriteint(SERVER, ev.chr)
    nflush(SERVER)
    end
  end

function netsendmouse(ev)
  if vialibtcod then
    tcodmouse(ev.x, ev.y, ev.state)
    end
  if SERVER then
    nwriteint(SERVER, nepMouse)
    nwriteint(SERVER, ev.x)
    nwriteint(SERVER, ev.y)
    nwriteint(SERVER, ev.state)
    nflush(SERVER)
    end
  end

function checkstreams()
  if SERVER and nready(SERVER) then
    if neof(SERVER) then
      delete(SERVER)
      SERVER = nil
      localmessage("server disconnected, press Ctrl+M, Ctrl+[ or F4 for NotEye menu")
      localmessage("then Q to quit")
    else
      readdata(SERVER, true, false)
      end
    return true
    end
  return checkclients()
  end

function onimagenotfound(s)
  logprint("image not found: "..s)
  s = string.gsub(s, ".png", "")
  local i = string.find(s, "gfx/")
  if i then s = string.sub(s, i+4) end
  local a,b,c = string.match(s, "^(.+)-(%d+)x(%d+)$")
  if a then
    s=a
    end
  logprint("trying to load font: "..s)
  if string.match(s, "Brogue") then 
    loadfont(s,16,16,fontBrogue)
  else
    loadfont(s,32,8,fontSimple)
    end
  if b or c then
    s = scalefont(a, b, c)
    end
  return Fonts[s].png
  end

-- == network menu ==

netmenu = {}

addtomenu(netmenu, "v", writechoicef(
  function()
    if VIEW then return "stop viewing"
    elseif SERVER then return "disconnect from the server"
    else return "view a recording"
    end end),
  
  function()
    if VIEW then
      delete(VIEW)
      VIEW = nil
    elseif SERVER then
      delete(SERVER)
      SERVER = nil
    else--if not P then
      local ret = askstr(viewfile, "enter the name of the file to view")
      if not ret then return false end
      viewfile = ret
      startviewing()
      end
    end
  )

addtomenu(netmenu, "c", writechoicef(function() 
    if VIEW then
      return "viewing speed: "..viewspeed
    elseif SERVER then
      return "disconnect from the server"
    else
      return "connect to a server"
      end end
    ),
  function()
    if VIEW then
      viewspeed = applymod(viewspeed, mod)
    elseif SERVER then
      delete(SERVER)
      SERVER = nil
    elseif not P then

      local ret = askstr(serverhost, "enter the host name")
      if not ret then return false end
      serverhost = ret

      ret = askstr("" .. portclient, "enter the port number")
      if not ret or not tonumber(ret) then return false end
      portclient = tonumber(ret)
      
      connecttoserver()
      
      end
      return 1
    end
  )

addtomenu(netmenu, "p", writechoicef(function()
  return servemode and "send P/T/R to viewers" or "let viewers decide P/T/R"
  end),
  function() servemode = not servemode return 1 end
  )

addtomenu(netmenu, "r", writechoicef( function()
  return REC and "finish recording" or "start recording to "..recfile end),
  function()
    if REC then
      delete(REC)
      REC = nil
    else
      local ret = askstr(recfile, "enter the name of the file to record")
      if not ret then return false end
      recfile = ret

      startrecording()
      if REC then
        sendscreen(REC)
        sendfacedir(REC)
        sendcursor(REC)
        sendmode(REC)
        nflush(REC)
        end
      end
    return 1
    end
  )

addtomenu(netmenu, "q", writechoicef(function() return "rec speed: "..recspeed end),
  function()
    recspeed = applymod(recspeed, mod)
    return 1
    end,
  function() return not REC end
  )

addtomenu(netmenu, "s", writechoicef(function()
    if ACCEPT then
      return "disable server ("..table.getn(CLIENTS)..")"
    elseif network == 1 then
      return "run a server on port "..portserver
    else
      return "server disabled"
      end end),  
  function()
    if network ~= 1 then
    elseif ACCEPT then
      stopserver()
    else
      ret = askstr("" .. portserver, "enter the port number", 1)
      if not ret or not tonumber(ret) then return false end
      portserver = tonumber(ret)
      
      startserver()
      end
    return 1
    end
  )

addtomenu(netmenu, "i", writechoicef(function()
  return serveinput and "accept input from clients" or "reject input from clients"
  end),
  function() 
    serveinput = not serveinput
    return 1
    end
  )

addtomenu(netmenu, "e", writechoice("erase messages from the screen"),
  function()
    messages = {}
    return 1
    end
  )

addtomenu(netmenu, "m", writechoice("add message"),
  function()
    local lmessage = askstr("", "Enter your message:")
    if lmessage then 
      if SERVER then
        nwriteint(SERVER, nepMessage)
        nwritestr(SERVER, lmessage)
        nflush(SERVER)
      else
        addmessage(lmessage) 
        end
      end
    return 1
    end,
  function() return gameon end
  )
  
addtomenu(mainmenu, "n", writechoice("network, recording and viewing"),
  function()
    menuexecute(netmenu, dfthdr("Network / recording / viewing"))
    end
  )

function connecttoserver()
  SERVER = nconnect(serverhost, portclient)
  if SERVER == 0 then
    SERVER = nil
    localmessage("Connection failed")
  else
    gamename = "auto"
    viewmode = true
    initviewer(SERVER)
    viewmode = false
    end
  end
  
function connecttotcod()
  SERVER = nconnect(libtcodhost, libtcodport)
  if SERVER == 0 then
    SERVER = nil
    print("No server!")
  else
    gamename = "auto"
    viewmode = true
    initviewer(SERVER)
    viewmode = false
    return true
    end
  end
  
addtomenu(mainmenu, "l", writechoice("connect to libtcod (Brogue etc)"),
  connecttotcod, nogameonly
  )

