-- Necklace of the Eye v6.5 roguelike frontend
-- Copyright (C) 2010-2013 Zeno Rogue, see COPYING for details

-- things related to fonts

if not Fonts then Fonts = {} end

function loadfont(fname, x, y, adjuster)
  if not Fonts[fname] then
    local F = {}
    local P = loadimage(gfxdir..fname..".png")
    if P == 0 then
      print("font not found: "..fname..", replacing by the default font")
      -- replace with 
      P = loadimage(gfxdir.."cp437-8.png")
      x = 16
      y = 16
      end
    local Psize = imggetsize(P)
    F.png = P
    
    F.font = adjuster(P,x,y,Psize.x,Psize.y)
    
    F.alias = imagealias(F.png, fname..".png")
    F.size = {x=Psize.x/x, y=Psize.y/y, rx=x, ry=y}
    Fonts[fname] = F
    end
  return fname
  end

function scalefont(fname, scx, scy)
  if scx == 1 and scy == 1 then
    return fname
    end
  local fname2 = fname.."-"..scx.."x"..scy
  if not Fonts[fname2] then
    local Psize = imggetsize(Fonts[fname].png)
    local P = newimage(Psize.x*scx, Psize.y*scy)
    local F = Fonts[fname].size
    imagetitle(P, fname2)
    local Psize = imggetsize(P)
    for y=0,Psize.y-1 do for x=0,Psize.x-1 do
      setpixel(P, x, y, getpixel(Fonts[fname].png, math.floor(x/scx), math.floor(y/scy)))
      end end
    Fonts[fname2] = {}
    Fonts[fname2].png = P
    Fonts[fname2].font = newfont(P, F.rx, F.ry, 0xFF000000)
    Fonts[fname2].size = {x=F.x*scx, y=F.y*scy, rx=F.rx, ry=F.ry}
    end
  return fname2
  end

function selectfont(fname)
  curfont = fname
  Font = Fonts[fname].font
  fontsize = Fonts[fname].size
  StreamSetFont = 2
  end

function fontSimple(P,fx,fy,sx,sy)

  for x=0,sx-1 do
  for y=0,sy-1 do
    local c = getpixel(P,x,y)
    -- c = colorset(c, 3, colorpart(c, 0))
    c = 0x1000001 * colorpart(c, 0)
    setpixel(P, x,y, c)
    end end

  return newfont(P, fx, fy, 0)
  end

function fontSimpleA(P,fx,fy,sx,sy)

  for x=0,sx-1 do
  for y=0,sy-1 do
    local c = getpixel(P,x,y)
    -- c = colorset(c, 3, colorpart(c, 0))
    c = 0x1000001 * colorpart(c, 0)
    setpixel(P, x,y, c)
    end end

  return newfont(P, fx, fy, transAlpha)
  end

function fontAlpha(P,fx,fy,sx,sy)
  return newfont(P,fx,fy,transAlpha)
  end

function fontBrogue(P,fx,fy,sx,sy)
  for x=0,sx-1 do
  for y=0,sy-1 do
    pix = getpixel(P, x,y)
    if bAND(pix, 0xFF) > 0x60 then pix = bAND(pix, 0xFF0000FF)
    else pix = 0
    end
    setpixel(P, x, y, pix)
    end end
  return newfont(P,fx,fy,transAlpha)
  end

function selectfontfull(fname)
  if fname then
    selectfont(fname)
    prepareconsole()
    if P then setfont(P, Font)
    elseif S then setfont(S, Font) end
    end
  end

-- == fonts menu ==

fontsmenu = {}

function addFont(key,name,a,b,c,d,menu)
  addtomenu(menu or fontsmenu,
    key,
    writechoice(name),
    function () 
      selectfontfull(loadfont(a,b,c,d)) 
      end
    )
  end

function addFontScaled(key,name,a,b,c,d,e,f)
  addtomenu(fontsmenu,
    key,
    writechoice(name),
    function () selectfontfull(scalefont(loadfont(a,b,c,d),e,f,d)) end
    )
  end

addFont("q", "VGA font 8x16 (DOS default)", "cp437-16", 32, 8, fontSimple)

addFont("a", "VGA font 8x14", "cp437-14", 32, 8, fontSimple)
addFont("z", "VGA font 8x8", "cp437-8", 32, 8, fontSimple)

addFont("w", "VGA font 12x24 (3/2 size)", "cp437-24", 32, 8, fontSimple)

addFontScaled("e", "VGA font 16x32 (double size)", "cp437-16", 32, 8, fontSimple, 2, 2)
addFontScaled("d", "VGA font 16x28 (double size)", "cp437-14", 32, 8, fontSimple, 2, 2)
addFontScaled("c", "VGA font 16x16 (double size)", "cp437-8",  32, 8, fontSimple, 2, 2)

addFont("t", "Fantasy font 16x32", "fantasy-32", 32, 8, fontSimpleA)
addFont("b", "Fantasy font 16x16", "fantasy-16s", 32, 8, fontSimpleA)
addFont("g", "Fantasy font 8x16", "fantasy-16", 32, 8, fontSimpleA)

addFont("u", "DejaVu Mono 10x20", "dejavu-20", 32, 8, fontSimpleA)

if havebrogue then for u=1,5 do
  addFont(""..u, "Brogue font #"..u,
    "BrogueFont"..u, 16, 16, fontBrogue
    )
  end end

addtomenu(mainmenu, "r", writechoice("select font and resolution"),
  function()
    menuexecute(fontsmenu, dfthdr("Select your font and resolution:"))
    end
  )

