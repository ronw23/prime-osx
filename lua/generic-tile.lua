-- Necklace of the Eye v5.1 roguelike frontend
-- Copyright (C) 2010-2011 Zeno Rogue, see COPYING for details

if not CursorTile then
  CursorImg = newimage(32, 32, 0)
  for x=0,31 do 
    setpixel(CursorImg, x, 0,  0xFF0000FF)
    setpixel(CursorImg, x, 31, 0xFF0000FF)
    setpixel(CursorImg, 0,  x, 0xFF0000FF)
    setpixel(CursorImg, 31, x, 0xFF0000FF)
    end
  CursorTile = 
    tilespatial(addtile(CursorImg, 0, 0, 32, 32, 0), spFlat + spWall + spFloor + spCeil + spIWall + spIFloor + spICeil)
  end

if FontBordered == nil and Gfx then

FontBordered = newimage(20*32, 36*8, 0x8000)
for c=0,255 do
  local ch = string.format("%c", c)
  local gch1 = fget(Font, ch)
  local gch2 = tilerecolor(gch1, 0)
  local cx = (c % 32) * 20
  local cy = math.floor(c / 32) * 36
  for x=0,4 do
    for y=0,4 do
      if (x-2)*(x-2)+(y-2)*(y-2) < 8 then
        drawtile(FontBordered, gch2, cx+x, cy+y, 16, 32)
        end
      end
    end
  drawtile(FontBordered, gch1, cx+2, cy+2, 16, 32)
  end

FontB = newfont(FontBordered, 32, 8, 0x8000)

function bord(x)
  return fget(FontB, x)
  end
end

outofscreen = tilefill(0)

function xtile(C,x,y)
  if C == 0 then return outofscreen end
  local ch = gch(C)
  local av = gchv(C)
  global_x = x
  global_y = y
  global_uu = (x+3*y) % 5
  global_co = gco(C)
  global_ba = gba(C)
  local aux = xtileaux(ch, global_co, av, global_ba)
  if drawcursor and global_x+mapregion.x0 == pcursor.x and global_y+mapregion.y0 == pcursor.y then
    aux = addgraphcursor(aux)
    end
  return aux
  end

function addgraphcursor(C)
  local Cur=tilerecolor(CursorTile, blinkcolor())  
  return tilemergeover(C, Cur)
  end

function rec(x) return tilerecolor(x, global_co) end
function rb(x) return rec(bord(x)) end

function floorcolid(co, id)
  co = colormix(co, 0)
  return 
    tilemerge(
      tilespatial(tilerecolor(WT[id][global_uu], co), spFlat + spFloor + spIFloor),
      tilespatial(tilerecolor(WT[4][global_uu], co), spCeil)
      )    
  end

function floorcol(co)
  return floorcolid(co, 10)
  end

function shadowfloor() 
  return floorcol(0x404040)
  end

function lavacol(co)
  return 
    tilemerge(
      tilespatial(tilerecolor(WT[4][global_uu], co), spFlat + spFloor + spIFloor),
      tilespatial(tilerecolor(WT[4][global_uu], colormix(co, 0)), spCeil)
      )
  end

function wallcolid(co, id)
  return 
    tilespatial(tilerecolor(WT[id][global_uu], co), spFlat + spWall + spIWall + spICeil)
  end

function wallcol(co)
  return wallcolid(co, 6)
  end

-- Vapors of Insanity tiles

function vaporat(x, y)
  if VaporPNG == nil then
    VaporPNG = loadimage(gfxdir.."vapors.png")
    VaporKey = getpixel(VaporPNG, 0, 0)
    VaporTab = { }
    end
  local id = x + y * 50
  if not VaporTab[id] then 
    VaporTab[id] = addtile(VaporPNG, x*32, y*32, 32, 32, VaporKey)
    end
    
  return VaporTab[id]
  end

function vaporrec(x, y)
  return rec(vaporat(x,y))
  end

function vaporatc(col)
  return function (x,y) return tilerecolor(vaporat(x,y), col) end
  end

function vaporatitem(x, y)
  return tilespatial(vaporat(x,y), spItem + spFlat + spIItem)
  end

-- roguelike tiles

function rlat(x, y)
  if not RLTilesPNG then
    RLTilesPNG = loadimage(gfxdir.."rltiles.png")
    RLTilesKey = getpixel(RLTilesPNG, 0, 0)
    RLTilesTab = { }
    end
  local id = x + y * 50
  if not RLTilesTab[id] then 
    RLTilesTab[id] = addtile(RLTilesPNG, x*32, y*32, 32, 32, RLTilesKey)
    end
  return RLTilesTab[id]
  end

function rlrec(x, y)
  return rec(rlat(x,y))
  end

function tspatm(base, set, spat, x, y)
  return tilemerge(base, tilespatial(set(x,y), spat))
  end

function tspat(set, spat, x, y)
  return tilemerge(shadowfloor(), tilespatial(set(x,y), spat))
  end

function tspatc(set, spat, x, y)
  return tilemerge(floorcol(global_co), tilespatial(set(x,y), spat))
  end

ssMon    = spFlat + spMonst + spIItem
ssItem   = spFlat + spItem + spIItem
ssWall   = spFlat + spWall + spIWall + spICeil
ssWallnc = spFlat + spWall + spIWall
ssFloor  = spFlat + spFloor + spIFloor
ssFloorv = spFlat + spFloor + spIItem

-- xtileaux

function xtileaux2(ch, co, av, ba)
  -- redefined by noefile
  end

function xtileaux(ch, co, av, ba)
  
  local spec = xtileaux2(ch, co, av, ba)

  if spec then
    return spec
  elseif ch == " " then 
    return tilefill(0)
  elseif av == 0 then 
    return tilefill(0)
  elseif av == 255 then
    return tilefill(0)
  elseif ch == (".") or av == 250 or av == 249 then
    return floorcol(co)
  elseif ch == ("#") or (av >= 176 and av <= 178) then
    return wallcol(co)
  elseif ch == (">") then
    return tspatc(rb, spFlat + spFloor + spIFloor, ch)
  elseif ch == ("<") then
    return tspatc(rb, spFlat + spCeil + spIFloor, ch)
  elseif (ch >= ("a") and ch <= ("z")) or (ch >= ("A") and ch <= ("Z")) then
    return tspat(rb, spFlat + spMonst + spIItem, ch)
  else 
    return tspat(rb, spFlat + spItem + spIItem, ch)
    end
  end

