-- Necklace of the Eye v6.5 roguelike frontend
-- Copyright (C) 2010-2013 Zeno Rogue, see COPYING for details

-- things related to audio

-- baksoundtrack backs up the soundtrack as reconfigured by user

soundtrackobj = {}

function resetmusic()
  soundtrack = {}
  end

function addmusic(s)
  table.insert(soundtrack, s)
  end

function clonetrack(src, dst)
  dst = {}
  for k,v in pairs(src) do
    dst[k] = v
    end
  end

if baksoundtrack then
  clonetrack(baksoundtrack, soundtrack)

else
  resetmusic()
  addmusic(sounddir.."music1.ogg")
  addmusic(sounddir.."music2.ogg")
  end

mid = 0

function checkmusic()
  if not musicon() then 
    if volmusic == 0 then return end

    if #soundtrack == 0 then
      return
      end
    local s = soundtrack[1 + mid % #soundtrack]
    mid = mid + 1
    if not soundtrackobj[s] then
      soundtrackobj[s] = loadmusic(s)
      end
    local obj = soundtrackobj[s]
    if not obj then
      return
      end

    musicvolume(volmusic)
    playmusic(obj)
    end
  end

function disablemusic()
  if not forcemusic then
    resetmusic()
    musicvolume(0)
    havemusic = false
    end
  end

-- loading and garbage-collection music 

Musics = {}
MusicTime = {}

function freeoldmusic(maximum)
  local maxt = os.clock()
  local q = 0
  for n,t in pairs(MusicTime) do
    if t < maxt then maxt = t end
    q = q + 1
    end
  for n,t in pairs(MusicTime) do
    if t == maxt and q > maximum then 
      print("freeing: "..n)
      delete(Musics[n])
      Musics[n] = nil
      MusicTime[n] = nil
      end
    end
  end

function loadmusic2(filename)
  if not Musics[filename] then
    local m = loadmusic(musicdir..filename)
    if m > 0 then Musics[filename] = m end
    end
  MusicTime[filename] = os.clock()
  end

addtomenu(mainmenu, "v", writechoicef(function()
    return "configure audio (sound volume "..volsound..", music volume "..volmusic..")" end),
  function()
    ret = askstr("" .. volsound, "enter the sound volume (1-128, or 0 to turn off):")
    if tonumber(ret) then volsound = tonumber(ret) end
    ret = askstr("" .. volmusic, "enter the music volume (1-128, or 0 to turn off):")
    if tonumber(ret) then volmusic = tonumber(ret) end
    musicvolume(volmusic)
    if volmusic == 0 then musichalt() end
    return 1
    end,
  function() return not havemusic end
  )

