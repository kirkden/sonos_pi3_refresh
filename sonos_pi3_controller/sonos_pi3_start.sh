#!/bin/sh

# Overall volume and Tweeter volume needs to be reduced 
amixer -M -D default sset Digital,0 80%,70%
alsactl store 

APPS_PATH=/home/tc/sonos_pi3_controller
PYTHONPATH=$APPS_PATH/lib exec python3 $APPS_PATH/sonos_pi3_controller.py $*
