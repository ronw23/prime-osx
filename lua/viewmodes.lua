-- Necklace of the Eye v5.2 roguelike frontend
-- Copyright (C) 2010-2012 Zeno Rogue, see COPYING for details
-- functions related to viewing

-- mode constants

modeASCII = 1
modeTiles = 2
modeFPP   = 3
modeISO   = 4
modeMini  = 5
modeMed   = 6

-- the default config, to be used in games by default
-- (called here, you usually don't need to call it yourself)
function dftconfig()

  -- size of the console
  if haveascii then
    conssize = scrgetsize(openconsole())
  else
    conssize = {x=80, y=25}
    end

  -- size of tiles to use in the tile mode
  tilesize = {x=32, y=32}
  
  -- which part of the process screen contains the map?
  mapregion = {x0=0, y0=0, x1=80, y1=19}

  -- where is the mini-map located?
  -- 0 is top left, 1 is bottom right, other values are proportional
  minipos = {x=1, y=1}

  -- how to place the ASCII screen if the resolution is larger than
  -- enough to fit it? 0.5 means center, 0 means top left, etc
  reloffset = {x=0.5, y=0.5}
  
  -- should the tiles fill the whole screen if the resolution is larger
  -- than the ASCII screen?
  fullextent = true
  
  -- which tile is used out of the map
  OutOfMap = 0
  
  -- what color to use for the border of the minimap?
  minimapborder = 0x808080
  
  -- which tile layers are used by the given game
  tilelayers = {0}
  
  -- parameters for the Isometric
  IsoParam = isoparam(32, 32, 32, 32)  
  end

dftconfig()

-- update the screen (copy from the internal buffer to the real screen)
function updscr()
  updaterect(0, 0, xscrsize.x, xscrsize.y)
  end

-- clear the internal buffer
function clrscr()
  fillimage(Gfx, 0, 0, xscrsize.x, xscrsize.y, 0)
  end

-- override this function if you want the screen size to be
-- greater than console size times font size
-- (this function works by modifying its parameter)
function setscreensize(xscrsize)
  end

-- called to change the video mode, and calculate all the parameters
function setvideomodex()
  if havegfx then
    xscrsize = {x = scrsize.x, y = scrsize.y + msgreserve * fontsize.y}
    setscreensize(xscrsize)
    if fscr then
      xscrsize = findvideomode(xscrsize.x, xscrsize.y)
      end
    local flag = 0
    if fscr  then flag = flag + SDL_FULLSCREEN end
    if useGL and opengl then flag = flag + SDL_OPENGL + SDL_HWSURFACE end
    local res = setvideomodei(xscrsize.x, xscrsize.y, flag)
    if res == false then
      -- probably the font was too big
      if fallback then 
        error("Error: unable to set any graphics mode")
        end
      fallback = true
      selectfontfull(loadfont("cp437-8", 32, 8))
      fallback = false
      end
    scroffset = {x=(xscrsize.x-scrsize.x)*reloffset.x, y = (xscrsize.y-scrsize.y)*reloffset.y}
    -- print("resolution used: "..xscrsize.x.."x"..xscrsize.y)
  
    prepareextra()
    
    fppvar = {
      vx0 = 0, vy0 = 0,
      vx1 = xscrsize.x, vy1 = xscrsize.y,
      vxm = xscrsize.x/2, vym = xscrsize.y/2,
      vxs = xscrsize.y/2, vys = xscrsize.y/2,
      vimg = 1
      }
  elseif hadgfx then
    closevideomode()
    end
  
  if haveascii then
    mainscreen = openconsole()
  elseif hadascii then
    delete(mainscreen)
    mainscreen = NIL
    end
    
  hadgfx = havegfx
  hadascii = haveascii
  end

-- as before, but also prepare the console (set size etc)

function prepareconsole()

  scrsize = {x=fontsize.x*conssize.x, y=fontsize.y*conssize.y}
  
  S = renewscreen(S, conssize.x, conssize.y)
  IMG  = renewscreen(IMG, conssize.x, conssize.y)
  IMGX = renewscreen(IMGX, conssize.x, conssize.y) -- auxiliary for shading
  
  mapsize = { x = mapregion.x1-mapregion.x0, y = mapregion.y1-mapregion.y0 }

  MAP = renewscreen(MAP, mapsize.x, mapsize.y)
  
  Tiles = renewscreen(Tiles, mapsize.x, mapsize.y)
  scrset(Tiles, 999, 999, OutOfMap)  
  
  Minimap = renewscreen(Minimap, mapsize.x+2, mapsize.y+2)
  scrfill(Minimap, 0, 0, mapsize.x+2, mapsize.y+2, tilefill(0))

  playerpos = {x=1, y=1}
  mapon = false

  setvideomodex()

  end

-- calculate the place where to put the center in the tile mode
function settilectr()
  tilectr = {x=xscrsize.x/2, y=xscrsize.y/2}
  end

-- calculate how many tiles to draw on the screen
function settileextent()
  if fullextent then
    tileext = {
      left=math.ceil(tilectr.x/tilesize.x),
      up=math.ceil(tilectr.y/tilesize.y),
      right=math.ceil((xscrsize.x-tilectr.x)/tilesize.x),
      down =math.ceil((xscrsize.y-tilectr.y)/tilesize.y)
      }
  else
    tileext = {
      left=math.ceil((tilectr.x-scroffset.x)/tilesize.x),
      up=math.ceil((tilectr.y-scroffset.y)/tilesize.y),
      right=math.ceil((scrsize.x+scroffset.x-tilectr.x)/tilesize.x),
      down =math.ceil((scrsize.y+scroffset.y-tilectr.y)/tilesize.y)
      }
    end
  end

-- prepare extra information after setting the video mode
function prepareextra()
  settilectr()
  settileextent()
  tileext.x = tileext.left+tileext.right+1
  tileext.y = tileext.up+tileext.down+1
  TileCut = renewscreen(TileCut, tileext.x, tileext.y)
  end

-- == drawing the ASCII screen ==

-- draw the transparent ASCII screen

function drawAsciiTransparent()

  scrcopy(IMG, 0,0, IMGX, 0,0, conssize.x, conssize.y, cellblack)
  
  local X = scroffset.x
  local Y = scroffset.y
  
  drawscreen(1, IMGX, X-1, Y, fontsize.x, fontsize.y)
  drawscreen(1, IMGX, X, Y-1, fontsize.x, fontsize.y)
  drawscreen(1, IMGX, X+1, Y, fontsize.x, fontsize.y)
  drawscreen(1, IMGX, X, Y+1, fontsize.x, fontsize.y)
  
  scrcopy(IMG, 0,0, IMGX, 0,0, conssize.x, conssize.y, celltransorshade)
  drawscreen(1, IMGX, X, Y, fontsize.x, fontsize.y)
  end

-- draw the semi-transparent ASCII screen

function drawAsciiSemitransparent()

  local X = scroffset.x
  local Y = scroffset.y

  drawscreenx(1, IMG, X, Y, fontsize.x, fontsize.y, cellbackshade75)

  scrcopy(IMG, 0,0, IMGX, 0,0, conssize.x, conssize.y, cellblack)
  drawscreen(1, IMGX, X-1, Y, fontsize.x, fontsize.y)
  drawscreen(1, IMGX, X, Y-1, fontsize.x, fontsize.y)
  drawscreen(1, IMGX, X+1, Y, fontsize.x, fontsize.y)
  drawscreen(1, IMGX, X, Y+1, fontsize.x, fontsize.y)

  drawscreenx(1, IMG, X, Y, fontsize.x, fontsize.y, celltrans)
  end

-- simply draw the ASCII display
function drawdisplayASCII()
  drawscreen(1, IMG, scroffset.x, scroffset.y, fontsize.x, fontsize.y)
  end

-- draw the ASCII display, but include layers

function drawdisplayASCII_layers()
  for k,v in pairs(tilelayers) do
    drawscreenx(1, IMG, scroffset.x, scroffset.y, fontsize.x, fontsize.y, getlayerX(v))
    end
  end

-- == extra stuff ==

-- also draw the messages

function drawmessage()
  while table.getn(messages)>0 and (messages[1].at < os.clock() or table.getn(messages) > 5) do
    table.remove(messages, 1)
    end
  
  local yy = table.getn(messages)
  
  if yy>0 then
    if havegfx then
      drawtile(1, tilefill(vgaget(1)), 0, xscrsize.y-fontsize.y*yy, xscrsize.x, fontsize.y*yy)
      Msgline = renewscreen(Msgline, 80, 1)
      for y=1,yy do
        scrfill(Msgline, 0, 0, 80, 1, 0)
        scrwrite(Msgline, 0, 0, messages[y].text, Font, vgaget(14))
        drawscreen(1, Msgline, 0, xscrsize.y-fontsize.y*(yy-y+1), fontsize.x, fontsize.y)
        end
      end
    if haveascii then
      for y=1,yy do
        scrwrite(mainscreen, 0, 24+y-yy, messages[y].text, Font, vgaget(14))
        end
      end
    end

  if conshelp and mousepos then
    local xm = mousepos.x
    local ym = mousepos.y
    local xc = math.floor(mousepos.x / fontsize.x)
    local yc = math.floor(mousepos.y / fontsize.y)
    local sg = scrget(S, xc, yc)
    local av,co,ba = gavcoba(sg)
    Msgline = renewscreen(Msgline, 80, 1)
    thelp = "at "..xc..","..yc.." ("..xm..","..ym..") char = "..gch(sg).."/"..av.." color = "..ashex(co).." back = "..ashex(ba)
    scrfill(Msgline, 0, 0, 80, 1, 0)
    scrwrite(Msgline, 0, 0, thelp, Font, vgaget(11))
    drawtile(1, tilefill(0x004000), 0, xscrsize.y-fontsize.y*(yy+1), xscrsize.x, fontsize.y)
    drawscreen(1, Msgline, 0, xscrsize.y-fontsize.y*(yy+1), fontsize.x, fontsize.y)
    end
  end

-- draw the MiniMap

function drawMiniMap()
  local msx = 3*(mapsize.x+2)
  local msy = 3*(mapsize.y+2)
  scrfill(Minimap, 0, 0, msx, msy, tilefill(minimapborder))
  scrcopy(MAP, 0,0, Minimap, 1,1, mapsize.x, mapsize.y, xminimap)
  if mode ~= modeMed then
    cminipos = {
      x = minipos.x*(scrsize.x-msx),
      y = minipos.y*(scrsize.y-msy)
      }
  else
    cminipos = {x=0, y=0}
    if playerpos.y > (mapregion.y0 + mapregion.y1) / 2 then
      cminipos.y = mapregion.y0 * fontsize.y
    else
      cminipos.y = mapregion.y1 * fontsize.y - msy
      end
    if playerpos.x > (mapregion.x0 + mapregion.x1) / 2 then
      cminipos.x = mapregion.x0 * fontsize.x
    else
      cminipos.x = mapregion.x1 * fontsize.x - msx
      end
    end
  cminipos.x2 = cminipos.x + msx
  cminipos.y2 = cminipos.y + msy
  drawscreen(1, Minimap, cminipos.x+scroffset.x, cminipos.y+scroffset.y, 3, 3)
  end

-- draw the stuff other than the map
function drawMiniAndStats()
  if mapon then
    drawMiniMap()
    drawAsciiTransparent()
  else
    drawAsciiSemitransparent()
    end
  end

-- if the game has a special background, then draw it
-- and return true
function drawBackground()
  return false
  end

-- == various graphical modes ==

function drawdisplayMini()
  if mapon then
    scrcopy(MAP, 0, 0, IMG, mapregion.x0, mapregion.y0, mapsize.x, mapsize.y, xtile)
    end
  if drawBackground() then
    drawAsciiSemitransparent()
  else
    drawdisplayASCII_layers()
    end
  end

function drawdisplayTile()
  if needfill then fillimage(Gfx, 0, 0, xscrsize.x, xscrsize.y, 0) end
  scrset(MAP, 999, 999, 0)
  scrcopy(MAP, playerpos.x-tileext.left, playerpos.y-tileext.up, TileCut, 0, 0, tileext.x, tileext.y, xtile)
  
  for k,v in pairs(tilelayers) do
    drawscreenx(1, TileCut, tilectr.x - (tileext.left + 0.5) * tilesize.x, 
      tilectr.y - (tileext.up + 0.5) * tilesize.y, tilesize.x, tilesize.y, getlayerX(v))
    end
  drawMiniAndStats()
  end

function drawdisplayMed()
  drawdisplayASCII()
  
  if not mapon then
    return
    end
  
  -- if needfill then fillimage(Gfx, 0, 0, xscrsize.x, xscrsize.y, 0) end
  local medsize = math.floor((mapsize.x+1)/2)
  local tx = playerpos.x - math.floor(medsize/2)
  if tx < 0 then tx = 0
  elseif tx + medsize > mapsize.x then tx = mapsize.x - medsize
  end
  
  MedTiles = renewscreen(MedTiles, medsize, mapsize.y)
  scrcopy(MAP, tx, 0, MedTiles, 0, 0, medsize, mapsize.y, xtile)

  for k,v in pairs(tilelayers) do
    drawscreenx(1, MedTiles, mapregion.x0 * fontsize.x, mapregion.y0 * fontsize.y,
      fontsize.x*2, fontsize.y, getlayerX(v))
    end

  playerat = {
    x = (((mapregion.x0+playerpos.x+0.5) - (mapregion.x0+tx)) * 2 + mapregion.x0) * fontsize.x,
    y = (playerpos.y+0.5) * fontsize.y
    }
  drawMiniMap()
  end

function drawdisplayFPP()

  fillimage(Gfx, 0, 0, xscrsize.x, xscrsize.y, 0)
  scrcopy(MAP, 0,0, Tiles, 0,0, mapsize.x, mapsize.y, xtile)
  fpp(fppvar, playerpos.x, playerpos.y, facedir * 45, Tiles)
  fppvar.dy = 2
  drawMiniAndStats()
  end

function drawdisplayIso()
  clrscr()
  scrcopy(MAP, 0,0, Tiles, 0,0, mapsize.x, mapsize.y, xtile)
  scrset(Tiles, 999, 999, OutOfMap)
  local isi = isosizes(IsoParam)

  local rminx = (0 - tilectr.x) / 2 / isi.floor
  local rminy = (0 - tilectr.y) / isi.floor
  local rmaxx = (xscrsize.x - tilectr.x) / 2 / isi.floor
  local rmaxy = (xscrsize.y - tilectr.y) / isi.floor

  local dminx = math.floor(rminx + rminy)
  local dminy = math.floor(rminy - rmaxx)
  local dmaxx = math.ceil(rmaxx + rmaxy)
  local dmaxy = math.ceil(rmaxy - rminx)
  
  for dx=dminx,dmaxx do
  for dy=dminy,dmaxy do
    local drx = tilectr.x-isi.x/2+isi.floor*(dx-dy)
    local dry = tilectr.y-isi.y/2+isi.floor*(dx+dy)/2
    if drx > -isi.x and dry > -isi.y and drx < xscrsize.x and dry < xscrsize.y then
      local sg = scrget(Tiles, playerpos.x+dx, playerpos.y+dy)
      local ip = isoproject(sg, IsoParam)
      drawtile(1, ip, drx, dry, isi.x, isi.y)
      end
    end end
  drawMiniAndStats()
  end
  
function drawdisplay()

  if fps_start then
    frametime = os.clock() - fps_start
    frames = frames + 1
    end

  if mode == modeASCII then drawdisplayASCII()

  elseif mode == modeMini then drawdisplayMini()

  elseif mode == modeMed then drawdisplayMed()

  elseif mode == modeTiles then drawdisplayTile()
  
  elseif mode == modeFPP then drawdisplayFPP()
  
  elseif mode == modeISO then drawdisplayIso()
  
  else drawdisplayASCII()
  end

  drawmessage()
  end

function drawandrefresh()
  profstart("drawandrefresh")
  if havegfx then
    drawdisplay()
    updaterect(0, 0, xscrsize.x, xscrsize.y)
    end
  if haveascii then
    scrcopy(IMG, 0, 0, mainscreen, 0, 0, conssize.x, conssize.y, eq)
    drawmessage()
    refreshconsole(pcursor.y, pcursor.x)
    end
  profend("drawandrefresh")
  end

-- convert a pixel to process coordinates (modify the ev.x and ev.y)

function pixeltoproc(ev)
  if mode == modeASCII or mode == modeMini then
    ev.x = math.floor((ev.x - scroffset.x) / fontsize.x)
    ev.y = math.floor((ev.y - scroffset.y) / fontsize.y)
  elseif mode == modeMed then
    ev.x = mapregion.x0 + playerpos.x + math.floor((ev.x - playerat.x + fontsize.x) / fontsize.x/2)
    ev.y = math.floor((ev.y - scroffset.y) / fontsize.y)
  elseif cminipos and ev.x > cminipos.x and ev.y > cminipos.y and ev.x < cminipos.x2 and ev.y < cminipos.y2 then
    ev.x = mapregion.x0 + mround((ev.x - cminipos.x) / 3)
    ev.y = mapregion.y0 + mround((ev.y - cminipos.y) / 3)
  elseif mode == modeTiles then
    ev.x = mapregion.x0 + playerpos.x + mround((ev.x - tilectr.x) / tilesize.x)
    ev.y = mapregion.y0 + playerpos.y + mround((ev.y - tilectr.y) / tilesize.y)
  elseif mode == modeFPP then
    return
  elseif mode == modeISO then
    local isi = isosizes(IsoParam)
    local rx = ev.x - tilectr.x
    local ry = ev.y - tilectr.y
    ry = ry / isi.floor
    rx = rx / 2 / isi.floor
    ev.x = mapregion.x0 + playerpos.x + mround(rx + ry)
    ev.y = mapregion.y0 + playerpos.y + mround(ry - rx)
    end
  end

-- convert a pixel to a direction (return integer)

function pixeltodir(ev)
  if mode == modeASCII or mode == modeMini then
    return vectodir((ev.x - scroffset.x - (playerpos.x+0.5) * fontsize.x) * fontsize.y, (ev.y - scroffset.y - (playerpos.y+0.5) * fontsize.y) * fontsize.x, fontsize.x*fontsize.y)
  elseif mode == modeMed then
    return vectodir((ev.x - playerat.x) * fontsize.y, (ev.y - playerat.y)*fontsize.x*2, fontsize.x*fontsize.y*2)
  elseif mode == modeTiles then
    return vectodir(ev.x - tilectr.x, ev.y - tilectr.y, tilesize.x)
  elseif mode == modeISO then
    local isi = isosizes(IsoParam)
    local rx = (ev.x - tilectr.x) / 2
    local ry = ev.y - tilectr.y
    return vectodir(rx + ry, ry - rx, isi.floor)
  else
    return vectodir(ev.x - fppvar.vxm, ev.y - fppvar.vym, 32)
    end
  end

-- message subsystem

messages = {}

-- add a message, only locally
function localmessage(str)
  message = str
  table.insert(messages, {at = os.clock() + 10, text = str})
  end

-- == graphical modes menu ==

modesmenu = {}

function addgraphmode(key, text, m, menu)
  addtomenu(menu or modesmenu,
    key,
    writechoicecol(text, function () return mode == m and 14 or 7 end),
    function () mode = m modepower = 10 end
    )
  end

addgraphmode("p", "plain ASCII only (look at the map)", modeASCII)
addgraphmode("t", "tile mode", modeTiles)
addgraphmode("f", "first person perspective", modeFPP)
addgraphmode("i", "isometric projection", modeISO)
addgraphmode("j", "mini-tile mode", modeMini)
addgraphmode("d", "double mini-tiles", modeMed)

addtomenu(mainmenu, "m", writechoice("select NotEye's graphical mode"),
  function()
    menuexecute(modesmenu, glheader)
    end
  )

addtomenu(mainmenu, "f", writechoice("switch fullscreen (also Alt+Enter)"),
  function() 
    fscr = not fscr
    setvideomodex()
    end
  )

if opengl then addtomenu(modesmenu, "o", writechoice("switch OpenGL usage"),
  function() 
    useGL = not useGL
    if useGL then localmessage("OpenGL activated")
    else localmessage("OpenGL deactivated") end
    setvideomodex()
    end
  ) end

function glheader()
  menuy = 8
  scrwrite(IMG, 1, 1, "Select your graphical mode:", Font, vgaget(14))

  scrwrite(IMG, 1, 3, "OpenGL is currently "..boolonoff(useGL), Font, vgaget(7))

  scrwrite(IMG, 1, 5, "Hint: OpenGL makes NotEye work much faster in the FPP mode", Font, vgaget(7))
  scrwrite(IMG, 1, 6, "(and possibly in other modes), but is less robust", Font, vgaget(7))
  end
