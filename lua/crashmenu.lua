-- Necklace of the Eye v6.3 roguelike frontend
-- Copyright (C) 2010-2012 Zeno Rogue, see COPYING for details

-- what to do on NotEye crash in NotEye-integrated roguelikes
-- such as "sample" and ADOM

crashmenu = {}

crashoff = true

addtomenu(crashmenu, "p", writechoice("continue playing"),
  function() 
    havemain = false
    startbyname(game_to_launch)
    crashoff = false
    end
  )

addtomenu(crashmenu, "s", writechoice("continue playing in the safe mode"),
  function() 
    havemain = false
    crashsafe = true
    startbyname(game_to_launch)
    crashoff = false
    end
  )

addtomenu(crashmenu, "r", writechoice("reload NotEye scripts"),
  function() 
    -- reset the engine's "crashed" flag...
    setcrashval(0)
    -- and cause the error to reload it
    error("reloading the scripts")
    end
  )

prepareconsole()

while crashoff do 
  menuexecute(crashmenu, 
    dfthdr("The NotEye script has crashed... what do you want to do?\n"..geterrormsg())
    )
  end
