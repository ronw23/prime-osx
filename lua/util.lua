-- Necklace of the Eye v5.2 roguelike frontend
-- Copyright (C) 2010-2012 Zeno Rogue, see COPYING for details
-- various basic utilities

function mround(x)
  return math.floor(x + 0.5)
  end

function renewscreen(S, x, y)
  if S then
    local oldsize = scrgetsize(S)
    if oldsize.x ~= x or oldsize.y ~= y then
      scrsetsize(S, x, y)
      end
    return S
  else
    S = newscreen(x, y)
    return S
    end
  end

if not vgaorig then
  vgaorig = {}
  for i=0,15 do vgaorig[i] = vgaget(i) end

  function vgareset()
    for i=0,15 do vgaset(i, vgaorig[i]) end
    end
  end

function file_exists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

function boolonoff(x)
  return x and "on" or "off"
  end

function curdate()
  return os.date("%Y-%m-%d-%H-%M-%S")
  end

function ashex(v)
  return string.format("%02X", v)
  end

-- table utilities

function table.copy(t)
  local t2 = {}
  for k,v in pairs(t) do
    t2[k] = v
  end
  return t2
end

function table.merge(t2, t)
  for k,v in pairs(t) do
    t2[k] = v
  end
  return t2
end

-- ascii value

function av(x)
  return string.byte(x,1)
  end

function avf(x)
  return string.byte(x,1)
  end

-- identity for scrcopy

function eq(C)
  return C
  end

-- translate vector to direction, for use in mouse support

do local dirtab = {3, 2, 1, 4, 0, 0, 5, 6, 7}

function vectodir(x, y, mzero)

  max = math.max(math.abs(x), math.abs(y))
  
  if max*2 > mzero then
    dx = 5
    if x > max/2 then dx = dx + 1 end
    if x < -max/2 then dx = dx - 1 end
    if y > max/2 then dx = dx + 3 end
    if y < -max/2 then dx = dx - 3 end
    
    return dirtab[dx]
    end
  return -1
  end
end

-- various cell transformations

function cellshade(x)
  if x == 0 then return 0 end
  return tilemerge(tileshade(gba(x)), gp2(x))
  end

function celltrans(x)
  return gp2(x)
  end

function celltransorshade(x)
  local ba = gba(x)
  if x == 0 or gchv(x) == 0 then
    return x
  elseif ba <= 0 then
    return gp2(x)
  else
    return tilemerge(tileshade(ba), gp2(x))
    end
  end

function cellblack(x)
  if bAND(gco(x),0xFFFFFF) == 0 then return 0
  else
    return tilecol(gp2(x), 0xFF000000, recMult)
    end
  end

function cellbackshade(x)
  if not x then return 0 end
  return tileshade(gba(x))
  end

function cellbackshade75(x)
  if not x then return 0 end
  return tilealpha(gba(x), 0xC0C0C0)
  end

function cellbackfull(x)
  if not x then return 0 end
  return tilefill(gba(x))
  end

function cellblacknb(x)
  return tilecol(x, 0xFF000000, recMult)
  end

function getlayerX(val)
  return function(x) return getlayer(x, val) end
  end

-- identity matrix
ffid = freeformparam(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)

-- two-sided identity matrix
ffid_0 = freeformparam(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)
freeformparamflags(ffid_0, 0, false)

-- profiling

if not profiling then profiling = {} profilingq = {} end

function profstart(x)
  if not profiling[x] then profiling[x] = 0 profilingq[x] = 0 end
  profiling[x] = profiling[x] - os.clock()
  profilingq[x] = profilingq[x] + 1
end

function profend(x)
  profiling[x] = profiling[x] + os.clock()
end

function profinfo()
  logprint("profile results:\n")
  for k,v in pairs(profiling) do
    logprint(k.."="..v.." ("..profilingq[k]..")\n")
    end
  logprint("\n")
  end

-- functions and constants which were once in the NotEye engine,
-- but were removed due to new features

function tilefill(x)
  return tilealpha(x, 0xFFFFFF)
  end

function tileshade(x)
  return tilealpha(x, 0x808080)
  end

function tilerecolor(x, y)
  return tilecol(x, y, recDefault)
  end

spIWall = spIWallL + spIWallR
spWall = spWallN + spWallE + spWallS + spWallW

function tiletransform(t1, dx,dy, sx,sy)
  return tilexf(t1,dx,dy,sx,sy,0,0)
  end

function tilemerge3(a,b,c)
  return tilemerge(tilemerge(a,b),c)
  end

function tilemerge4(a,b,c,d)
  return tilemerge(tilemerge3(a,b,c),d)
  end

