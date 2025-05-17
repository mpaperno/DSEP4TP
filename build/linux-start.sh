#!/usr/bin/env bash

########################################################################
##
## Used to start the Plugin on Mac or Linux due to needing the execute
## permission and sometimes it not being retained properly while zipped
##
########################################################################

prog=$( dirname -- "$0"; )/bin/DSEP4TP
chmod +x $prog
$prog &
