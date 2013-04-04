#!/bin/sh
# based on the ArchLinux script for Hydra Slayer, by Joao Cordeiro <jlcordeiro at gmail dot com>

pwd="`pwd`"
export NOTEYEDIR="/usr/share/noteye"
 
if [ -z "$XDG_CONFIG_HOME" ]; then
    export NOTEYECONFIG="$HOME/.config/hydraslayer"
else
    export NOTEYECONFIG="$XDG_CONFIG_HOME/noteye"
fi   

mkdir -p "$NOTEYECONFIG" || return 1

test -e "$NOTEYECONFIG/config.noe" || cp "$NOTEYEDIR/common/dftconfig.noe" "$NOTEYECONFIG/config.noe"

$NOTEYEDIR/noteye -f $NOTEYECONFIG/hydra.sav -t $NOTEYECONFIG/hydralog.txt $* -N -C $NOTEYECONFIG/config.noe -P "$pwd"
