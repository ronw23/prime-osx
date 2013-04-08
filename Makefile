# to build a multi-user version, make clean and then make game-multiuser

OBJS = AttackType.o BOFH.o BunkerRooms.o Canister.o Cave.o Creature.o Droid.o\
Equipment.o Fight.o FloppyDisk.o Game.o Help.o Hero.o Implant.o Interface.o  \
Inventory.o Lore.o LoreHelp.o Mainframe.o MatterCompiler.o Map.o Menu.o      \
Monster.o MonsterAI.o Mutant.o NCMenu.o NCUI.o Object.o ObjectParser.o       \
OperatingSystem.o Options.o Path.o Profession.o RayGun.o Room.o SaveLoad.o   \
Services.o Sewer.o Shop.o Skills.o SpecialLevel.o Tombstone.o Tool.o         \
TwistyRooms.o Util.o Vat.o Vision.o Weapon.o Wreck.o main.o osx/macsupport.o
NEOBJS = NEMenu.o NEUI.o
GENFILES = Attack.cpp Attack.h Flavor.cpp Flavor.h MonsterIlk.cpp MonsterIlk.h\
ObjectIlk.cpp ObjectIlk.h

PROGRAM=prime
PROGRAM_OSX=PRIME.app

GAMEOWNER=root:games
PREFIX=
BINDIR=$(PREFIX)/usr/games/bin
MANDIR=$(PREFIX)/usr/share/man/man6
DATADIR=$(PREFIX)/usr/games/share/$(PROGRAM)
USERDIR=~/.config/$(PROGRAM)/
SCOREDIR=$(PREFIX)/var/games/$(PROGRAM)

# It is safe to leave this flag blank.
# The only case it should be set is if you are cross-compiling for mac os x.
#
# If set, and if you have the libraries to compile against, this will build 
# a binary that should work on 32/64 bit intel-only mac osx leopard and later.
#
#OSXFLAGS=-arch i386 -mmacosx-version-min=10.5
OSXFLAGS=-DOSX

#LDFLAGS=$(LDPATH) $(OSXFLAGS)
LIBS=-lpanel -lcurses
NELIBS=-lpanel -lcurses -lnoteye -lz -llua -framework Cocoa
LDFLAGS=$(LDPATH) $(OSXFLAGS)
NELDFLAGS=-L. -Xlinker 

CXX=g++

CXXFLAGS=-Wall -Wextra -pedantic -Wno-unused-parameter -O $(OSXFLAGS)

all: single-osx	

single-osx: osx-user $(PROGRAM) prepare-osx
single: single-user $(PROGRAM) prepare
console: single-user nogui $(PROGRAM)_con prepare
multi: multi-user $(PROGRAM)
multiconsole: multi-user nogui $(PROGRAM)_con

osx-user:
	echo "/* File generated by Make. */" > config.h
	echo "#define DATADIR \"../Resources/user\"" >> config.h
	echo "#define USERDIR \"../Resources/user\"" >> config.h
	echo "#define SCOREDIR \"../Resources/user\"" >> config.h

single-user:
	echo "/* File generated by Make. */" > config.h
	echo "#define DATADIR \"user\"" >> config.h
	echo "#define USERDIR \"user\"" >> config.h
	echo "#define SCOREDIR \"user\"" >> config.h

multi-user:
	echo "/* File generated by Make. */" > config.h
	echo "#define DATADIR \"$(DATADIR)\"" >> config.h
	echo "#define USERDIR \"$(USERDIR)\"" >> config.h
	echo "#define SCOREDIR \"$(SCOREDIR)\"" >> config.h

install:
	mkdir -p $(BINDIR)
	cp $(PROGRAM) $(BINDIR)
	mkdir -p $(MANDIR)
	cp docs/man_page $(MANDIR)/$(PROGRAM).6
	mkdir -p $(DATADIR)
	cp help/* $(DATADIR)
	cp data/* $(DATADIR)
	chown $(GAMEOWNER) $(DATADIR)
	mkdir -p $(SCOREDIR)
	touch $(SCOREDIR)/logfile.txt
	touch $(SCOREDIR)/highscores.dat
	chown -R $(GAMEOWNER) $(SCOREDIR)
	chmod -R 775 $(SCOREDIR)
	gzip $(MANDIR)/$(PROGRAM).6

uninstall:
	rm $(BINDIR)/$(PROGRAM)
	rm $(MANDIR)/$(PROGRAM).6.gz
	rm $(SCOREDIR)/logfile.txt
	rm $(SCOREDIR)/highscores.dat
	rmdir --ignore-fail-on-non-empty $(SCOREDIR)
	rm $(DATADIR)/*
	rmdir --ignore-fail-on-non-empty $(DATADIR)

prepare:
	strip $(PROGRAM)
	mkdir -p build
	mkdir -p build/$(PROGRAM)
	mkdir -p build/$(PROGRAM)/user
	mkdir -p build/$(PROGRAM)/shot
	cp $(PROGRAM) build/$(PROGRAM)/
#	cp docs/Guide.txt docs/Credits.txt build/$(PROGRAM)
	cp config.lua libnoteye.dylib build/$(PROGRAM)
	cp -r lua gfx build/$(PROGRAM)
	cp help/* build/$(PROGRAM)/user
	cp data/* build/$(PROGRAM)/user
	
prepare-osx:
	mkdir -p "build/$(PROGRAM_OSX)"
	mkdir -p "build/$(PROGRAM_OSX)/Contents"
	mkdir -p "build/$(PROGRAM_OSX)/Contents/MacOS"
	mkdir -p "build/$(PROGRAM_OSX)/Contents/Frameworks"
	mkdir -p "build/$(PROGRAM_OSX)/Contents/Resources"
	mkdir -p "build/$(PROGRAM_OSX)/Contents/Resources/user"
	mkdir -p "build/$(PROGRAM_OSX)/Contents/Resources/shot"
	cp $(PROGRAM) "build/$(PROGRAM_OSX)/Contents/MacOS/$(PROGRAM)"
	cp libnoteye.dylib "build/$(PROGRAM_OSX)/Contents/Frameworks"
	cp /usr/local/lib/liblua.5.1.5.dylib "build/$(PROGRAM_OSX)/Contents/Frameworks"
	install_name_tool -change libnoteye.dylib "@executable_path/../Frameworks/libnoteye.dylib"  "build/$(PROGRAM_OSX)/Contents/MacOS/$(PROGRAM)" 
	install_name_tool -change /usr/local/lib/liblua.5.1.5.dylib "@executable_path/../Frameworks/liblua.5.1.5.dylib"  "build/$(PROGRAM_OSX)/Contents/MacOS/$(PROGRAM)" 
	cp osx/Info.plist "build/$(PROGRAM_OSX)/Contents"
	cp -r lua gfx "build/$(PROGRAM_OSX)/Contents/Resources"
	sed -i '' -e "s/lua\/noteye/..\/Resources\/lua\/noteye/g" "build/$(PROGRAM_OSX)/Contents/Resources/lua/prime.lua"
	sed -i '' -e "s/config.lua/..\/Resources\/config.lua/g" "build/$(PROGRAM_OSX)/Contents/Resources/lua/noteye.lua"
	cp config.lua "build/$(PROGRAM_OSX)/Contents/Resources"
	cp help/* "build/$(PROGRAM_OSX)/Contents/Resources/user"
	cp data/* "build/$(PROGRAM_OSX)/Contents/Resources/user"

$(PROGRAM): config.h libnoteye.dylib $(GENFILES) $(OBJS) $(NEOBJS)
	$(CXX) -g -o $(PROGRAM) $(LDFLAGS) $(NELDFLAGS) $(OBJS) $(NEOBJS) $(NELIBS)

nogui:
	echo "#define NOGUI" >> config.h

$(PROGRAM)_con: config.h $(GENFILES) $(OBJS)
	c++ -g -o $(PROGRAM)_con $(LDFLAGS) $(OBJS) $(LIBS)

clean:
	rm -f $(PROGRAM) *.o core

cleaner: clean
	rm -f support/encyclopedia2c noteye/libnoteye.o libnoteye.dylib
	cd support/tablemaker && $(MAKE) clean

cleangen:
	rm -f $(GENFILES)

Attack.h Attack.cpp: Attacks.txt
	$(MAKE) support/tablemaker/tablemaker
	m4 < Attacks.txt | support/tablemaker/tablemaker
Flavor.h Flavor.cpp: Flavor.txt
	$(MAKE) support/tablemaker/tablemaker
	m4 < Flavor.txt | support/tablemaker/tablemaker
MonsterIlk.h MonsterIlk.cpp: Monsters.txt
	$(MAKE) support/tablemaker/tablemaker
	m4 < Monsters.txt | support/tablemaker/tablemaker
ObjectIlk.h ObjectIlk.cpp: Items.txt
	$(MAKE) support/tablemaker/tablemaker
	support/tablemaker/tablemaker < Items.txt
AttackType.o: Attack.h Attack.cpp
Monster.o: MonsterIlk.h MonsterIlk.cpp
Object.o: Flavor.h Flavor.cpp ObjectIlk.h ObjectIlk.cpp
Lore.h Lore.cpp: Lore.txt
	$(MAKE) support/encyclopedia2c
	support/encyclopedia2c Lore.txt
support/encyclopedia2c: support/encyclopedia2c.cpp
	$(CXX) support/encyclopedia2c.cpp -o support/encyclopedia2c
support/tablemaker/tablemaker:
	cd support/tablemaker && $(MAKE)
libnoteye.dylib:
	cd noteye && $(MAKE)
