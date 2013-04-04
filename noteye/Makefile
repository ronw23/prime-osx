#all: ../noteye
all: ../libnoteye.dylib

#for development:
#CFLAGS=-Wall -Werror
#for release:
SDLFLAGS=`sdl-config --cflags`
CFLAGS=-O9 $(SDLFLAGS)

#DEF=-DHYDRA -DLINUX -DOPENGL
DEF=-DLINUX -DOPENGL -DMAC

#noteye: noteye.cpp noteye.h util.cpp screen.cpp tiles.cpp gfx.cpp linux.cpp image.cpp fpp.cpp font.cpp internal.cpp stream.cpp
#	g++ noteye.cpp -o noteye -lSDL -lSDL_image -llua5.1 -lutil -DLINUX -lSDL_net -lz

libnoteye.o: libnoteye.cpp noteye.h util.cpp screen.cpp tiles.cpp gfx.cpp linux.cpp image.cpp fpp.cpp font.cpp internal.cpp stream.cpp consout.cpp sound.cpp lua.cpp iso.cpp noteye-curses.h opengl.cpp drawtile.cpp
	g++ -fpic libnoteye.cpp -c -o libnoteye.o $(CFLAGS) $(DEF)

../libnoteye.dylib: libnoteye.o
	g++ -shared -Wl,-install_name,libnoteye.dylib -o ../libnoteye.dylib libnoteye.o -framework SDL -framework SDL_image -llua -lutil -framework SDL_mixer -framework SDL_net -framework OpenGL -lz -lcurses

hydra.o: noteye-curses.h ../hydra/classes.cpp ../hydra/hydra.cpp ../hydra/mainmenu.cpp ../hydra/ui.cpp ../hydra/tutorial.cpp ../hydra/weapons.cpp ../hydra/data.cpp ../hydra/save.cpp ../hydra/achieve.cpp ../hydra/utils.cpp ../hydra/level.cpp ../hydra/monster.cpp ../hydra/drawhydras.cpp
	g++ ../hydra/hydra.cpp -c -o hydra.o -DNOTEYE $(CFLAGS)

../noteye: ../libnoteye.dylib hydra.o noteye-main.cpp
	g++ $(DEF) -Wl,-rpath,. -Wl,-rpath,/usr/share/noteye hydra.o noteye-main.cpp -o ../noteye ../libnoteye.dylib -lGL -lGLU

#../noteye: ../libnoteye.dylib noteye-main.cpp noteye.h
#	g++ -Wl,-rpath,. noteye-main.cpp -o ../noteye ../libnoteye.dylib

DESTDIR = /usr/share/noteye

install: ../noteye
	strip ../noteye
	install -T -D -m 755 ../noteye "$(DESTDIR)/noteye"
	install -T -D -m 755 ../libnoteye.dylib "$(DESTDIR)/libnoteye.dylib"
	install -D -m 755 calleri.sh "$(DESTDIR)/caller.sh"
	install -D -m 755 rogue.sh "$(DESTDIR)/rogue"
	mkdir -p $(DESTDIR)/gfx
	mkdir -p $(DESTDIR)/sound
	mkdir -p $(DESTDIR)/common
	mkdir -p $(DESTDIR)/games
	bash -c "cd ../gfx; install -D -m 744 -t \"$(DESTDIR)/gfx/\" *.png *.jpeg"
	bash -c "cd ../sound; install -D -m 744 -t \"$(DESTDIR)/sound/\" *.ogg"
	bash -c "cd ..; install -D -m 744 -t \"$(DESTDIR)/common\" common/*.noe"
	bash -c "cd ..; install -D -m 744 -t \"$(DESTDIR)/games\" games/*.noe"
	touch "$(DESTDIR)/hydrascores.sav"
	chmod 666 "$(DESTDIR)/hydrascores.sav"
	install -T -D -m 755 hydraslayer.sh "/usr/bin/hydraslayer"
	install -T -D -m 755 noteye.sh "/usr/bin/noteye"

uninstall:
	rm -rf /usr/share/noteye /usr/bin/noteye /usr/bin/hydraslayer

clean:
	rm *.o

# note about the install script:
# unfortunately I cannot set the group ID to games since it would cause a security hole (NotEye is scriptable)
# I don't know how to solve this (other than creating a separate hydra slayer executable), thus I simply make 
# the scores writeable by anyone

# uninstall will remove the global highscores!

sample: sample.cpp ../libnoteye.dylib noteye.h noteye-curses.h
	g++ -Wl,-rpath,. sample.cpp -o ../sample ../libnoteye.dylib
