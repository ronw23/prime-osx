dofile(commondir.."generic-tile.lua")

-- press a..y, A..Y for 50 lines of rltiles.png
-- press '1' and '2' to switch between RL tiles and Vapors tiles

function tiletest()
  local line = 0

  supdscr = updscr
  
  local gfun = rlat
  local linemax = 49
  local lines = 60
  
  local function charat(x, y)
    if y >= 8 then
      return tilefill(vgaget(x % 16))
      end
    local id = y*32+x
    local ch = string.char(id)
    return bord(ch)
    end
  
  function updscr() 
    for id = 0, linemax do
      local xx = (id%10) * fontsize.x * 8
      local yy = ((id-id%10)/10* 5 + 1) * fontsize.y
      drawtile(1, gfun(id, line), xx, yy, fontsize.x * 8, fontsize.y * 4)
      end
    supdscr()
    end
    
  while true do
    line = line % lines
    scrfill(IMG, 0, 0, conssize.x, conssize.y, 0)

    for id = 0, linemax do
      scrwrite(IMG, (id%10)*8, (id-id%10)/10*5, "("..id..","..line..")", Font, vgaget(15))
      end
      
    viewmenu()

    lev = getevent()
    if lev.type == evKeyDown then
      if lev.chr >= 48 and lev.chr < 58 then 
        line = lev.chr - 48
      elseif lev.chr >= 97 and lev.chr <= 122 then 
        line = lev.chr - 97 + 10
      elseif lev.chr >= 65 and lev.chr <= 90 then 
        line = (lev.chr - 65) + 36
      elseif lev.symbol == SDLK_F1 then
        gfun = rlat
        linemax = 49
        lines = 60
      elseif lev.symbol == SDLK_F2 then
        gfun = vaporat
        linemax = 17
        lines = 15
      elseif lev.symbol == SDLK_F3 then
        gfun = charat
        linemax = 31
        lines = 9
      elseif lev.symbol == SDLK_RETURN or lev.symbol == 27 then 
        updscr = supdscr
        return
        end
    elseif checkstreams() then
    elseif lev.type == 0 then
      sleep(10)
      end
    end

  end

tiletest()
