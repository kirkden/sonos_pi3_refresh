

# Sonos pi:3

------

[TOC]

------

## Section 1: Introduction

Due to the 'unprecedented times' I have had a lot more time to get jobs done around the home. One thing high on the "erk'ed me" list was the lovely sounding Sonos Play:3 Gen 1 that had fallen into disuse except possibly as a door stop.  The reason was it's lack of feature set and ability to integrate into my home audio system based on Logitech's open source Media Server (LMS).   Key feature I was after was synchronisation and after lots of playing around with various software bridges I came to the conclusion that the play:3 would need to be upgraded or stay as a doorstop.  So upgrade it was, and I am very pleased with the result.  

How does it work:

Quite simply I replaced the Sonos logic board with a Raspberry Pi 4 and HiFi Berry Amp running piCorePlayer (with Logitech Media Server)  keeping the Sonos P/S speakers and chassis.   Simple....

New Features include:

- *60W Class D Amplifier*
- *Sample Rate 44.1-192kHz (16-32bit)*
- *Multi Room audio + Bluetooth, USB* 
- *LADSP Plugin support*
- *Software DSP crossover*
- *Ability to do Room EQ'ing* 
- *Button control of play, pause, volume +/-, Jump forward/back*
- *LEDs indicate pause, play, volume +/-, network error, LMS server error*
- *LEDs reflects changes made from either physical buttons or web apps*   
- *Utilises hardware mute if supported and configured*
- *Bluetooth pairing button* 
- *Sync'ed players integration (pause will effect all)*
- *MQTT for pause - I use this to mute on doorbell ring* 
- *Serial console to monitor booting*
- *External USB for LAN, USB stick etc*    
- *Uses asyncio to improve response times*      
- *App support, Spotify, Tidal etc*
- *Is reversible, just in case I wanted the original Sonos play:3 back*
- High Quality power supply 





------

## Section: 2 Installation

This section documents how to achieve a similar result.   You dont need a lot of skill, just plenty of time and patience

**<u>Skills:</u>**

- Easy dismantle/assembly skills
- Basic soldering
- RaspberryPi/Linux command line
- Patience

<u>**Software**</u>:

- piCorePlayer (https://www.picoreplayer.org/)
- logitech media server installation - Lots of ways to do this, I use Docker, you can use the one that comes with picoreplayer, although I havent tested.
- sonos_pi3_controller and sonos_pi3_crossover (Details below)
- ssh client like PuTTY (https://www.putty.org/)

### Part A: Software

This can and should be done before you assemble into the Sonos case.  You can also test using the audio line out.

Sequence:

1. Install piCorePlayer
2. Install DSP, crossover and Eq filter
3. Update ALSA config
4. Test
5. Install button and led controller
6. Setup Serial console (optional)

#### Step 1 - Install piCorePlayer

​        Install piCorePlayer per instructions on web site (https://docs.picoreplayer.org/getting-started/)
​        and test that it connects to network, plays music and Bluetooth is working.

#### Step 2 - Installing DSP & Crossover

Using ssh client login as tc/piCore

Mount mmcblk0p2, change directory, install git  and clone this repository       

```
$ m2  
$ cd /mnt/mmcblk0p2/tce
$ tce-load -wi git    
$ git clone https://github.com/kirkden/sonos_pi3_refresh.git
```

You have a choice here, Option 1: you can compile all the plugins and DSP framework or Option 2: you can install the pre-compiled tcz (tiny core extension).   If your running on piCorePlayer I suggest just installing the tcz, option 2, way easier.
        

**Option 1: Compile ladspa and plugins** 

```
$ sudo tce-load -wi compiletc cmake squashfs-tools
$ cd /mnt/mmcblk0p2/tce/sonos_pi3/sonos_crossover/src
$ sudo make clean
$ sudo make all
$ sudo make tgz
```

Then follow the install steps below.

**Option 2: Install pre-compiled tcz extension**

```
$ cd /mnt/mmcblk0p2/tce/sonos_pi3_refresh/sonos_pi3_crossover/src
$ sudo sh ./xover-install.sh 
```

**<u>Note:</u>**  

*The install step will do 3 things.*

1. Copy sonos-pi3-plugins.tcz to /mnt/mmcblk0p/tce/optional/sonos-pi3-plugins.tcz to /mnt/mmcblk0p2/tce/onboot.lst
2. Copy all the ladspa plugins (Eq etc)
3. And do a backup, so when you reboot everything is there

#### Step 3 - Update alsa configuration

Next step is to update the ALSA configuration to add the DSP crossover and filters for the room Eq.  Firstly lets talk about the hardware config.  The Sonos play:3 has 2 x Mid range speakers and a tweeter  powered by three amplifiers.  In this project we will only use a stereo amp, so the I have configured the left channel to drive the two Mids connected in series and the right channel drives the tweeter.  The consequence of this is that the Sonos play:3 will be operating in mono, that said the final config is up to you.  So back to the ALSA config, the changes we are about to make create a DSP effect chain like this:  
        
    Squeezelite  
      |----> DSP EQ -> Mono Downmix -> Crossover -> Amp2+ 
    Speakers/Bluetooth

**Steps:**

Note: make a copy of your /etc/asound.conf as a backup just in case

```
$ cd ../dsp_crossover_config

# Edit asound.conf-template and ensure that the slave.pcm device in the 
# pcm.ladspa_testi (line 19) and pcm.crossover (line 44)definitions are 
# set to your audio device.
#    eg. slave.pcm "plughw:sndrpihifiberry" or 
#    slave.pcm "default"

$ sudo bash -c 'cat asound.conf-template >> /etc/asound.conf'

# Create a default Eq filter file
$ sudo install -m644 -D dsp-config-template /etc/ladspa_dsp/config
			
# Add it to the list of backup files
$ sudo grep -qxF 'etc/ladspa_dsp/config' /opt/.filetool.lst || echo 'etc/ladspa_dsp/config' >> /opt/.filetool.lst

# Now do a Backup.
$ pcp backup          
```

#### Step 4 - Testing

At this point, I suggest you reboot and test, everything should work like before.  The default EQ filters and crossover are set up, so you should hear different from the different effects between left and right using the examples below. 

```
$ pcp reboot
			
# login as tc/piCore and test

$ speaker-test -c 2 -t wav -D p_ladspa_test
$ speaker-test -c 2 -t wav -D downmix_mono
$ speaker-test -c 2 -t wav -D p_crossover
$ speaker-test -c 2 -t wav -D room_eq
```

​       

Assuming all works lets change squeezelite and bluetooth alsa player to point to the first effect in the chain - "room_eq"
          

```
$ sudo sed -i '/^OUTPUT/c\OUTPUT="room_eq"' /usr/local/etc/pcp/pcp.cfg
$ pcp bu
$ pcp reboot
```

Login again as tc/piCore and check squeezelite process
​        

```
$ ps -ef
	/usr/local/bin/squeezelite -n sonos-pi:3 -o room_eq -a 80 4  1  -C 5       
# If you see the "-o room_eq" - Everything is setup OK and you should be able to play music from
# squeezelite. If not check the pcp config and change it manually
$ sudo vi /usr/local/etc/pcp/pcp.cfg 
```


 If you are experimenting with headphones the left side should be bassy and the right squeaky.

#### Step 5 - Installing button and led controller

This is a little python application that will run in the background and respond to button presses,

| Short Press | Long Press        |
| ----------- | ----------------- |
| Play/Pause  | Bluetooth Pairing |
| Volume +    | Jump Forward      |
| Volume -    | Jump Backward     |

 and flashes the Sonos play:3 leds in accordance with the player state.

| Led Color | Function              |
| --------- | --------------------- |
| Green     | Pause                 |
| Red       | Error (internet, LMS) |
| Orange    | Pairing               |
| White     | Volume changes        |



First we need to load python and the GPIO extensions

```
$ tce-load -wi python3.6 RPi-GPIO-python3.6.tcz
```

This app will be run from the user tc's home directory

```
$ cd /mnt/mmcblk0p2/tce/sonos_pi3_refresh/
$ cp -rv sonos_pi3_controller /home/tc/
$ cd /home/tc/sonos_pi3_controller
```

Next edit the config.ini and change player_name, lms_ip and pin set up to reflect your system config, by default of course this is set up for sonos pi:3.

**<u>Notes:</u>**  

    If you dont use leds or buttons, just set it to an unused GPIO pin
    
    Important dont make the LED pin the same as the button pin or one will trigger the other

   2. If you dont want the network check or mqtt just leave them blank
         lms_port is typically 9090 by default, it is NOT web interface 9000 

         ```
         [pin]
         play_pause			= 11            # GPIO 17
         vol_down			= 13            # GPIO 27
         vol_up				= 15            # GPIO 22
         mute				= 7             # GPIO 4 HiFiBerry Amp2+ uses GPIO 4 as a Hardware Mute pulling low will mute the output
         
         [led]
         led1				= 18            # GPIO 24 Volume activity (White on sonos-pi3)
         led2				= 29            # GPIO 5  Play/Pause (Green on sonos-pi3)
         led3				= 31            # GPIO 6  Error (Red on sonos-pi3)
         led4				= 33            # GPIO 13 Pairing (Orange on sonos-pi3)
         
         [network]
         check_ip			= 1.1.1.1       # Public DNS
         check_port			= 53
         
         [app]
         log_level			= info
         long_press			= 1200          # in msecs
         backup_cmd			= [/usr/local/bin/pcp, bu]
         
         [lms]
         lms_ip				= 192.168.1.1
         lms_port			= 9090
         vol_step			= 5             # Step size for volume +/-   
         player_name			= sonos-pi:3    # use this to test for no device found
         
         [bluetooth]
         bluetoothctl_path	= /usr/local/bin/bluetoothctl
         bt_timeout			= 120
         bt_config_path		= /usr/local/etc/pcp/pcp-bt.conf
         bt_restart_cmd		= [/usr/local/etc/init.d/pcp-bt6, restart]
         
         [mqtt]
         mqtt_ip				= 192.168.1.1
         mqtt_port			= 1883
         mqtt_user			= mqtt
         mqtt_pass			= password
         mqtt_topic			= squeezelite/pause   # If undefined defaults to squeezelite/pause 
         ```
         
         Now check the startup shell script (sonos_pi3_start.sh - see below) and modify if required
         

```
#!/bin/sh
#  Overall volume and Tweeter volume needs to be reduced 

amixer -M -D default sset Digital,0 80%,70%
alsactl store 

APPS_PATH=/home/tc/sonos_pi3_controller
PYTHONPATH=$APPS_PATH/lib exec python3 $APPS_PATH/sonos_pi3_controller.py 
```

**<u>Note:</u>** 

Because I have wired the two woofers in series on the left channel (8 ohms) and the tweeter to the right (4 ohms) the tweeter is significantly louder and needs to be compensated through alsa mixer balance.   

Additionally, the HiFiBerry Amp2+ is 60watts which is way louder than I am comfortable with so I crank it down to 80% right and 60% left

Suggest you test it now.   By default it will print out the config and the connection messages.  You can add --log_level=debug if you need lots of output 
           

```
$ chmod +x ./sonos_pi3_start.sh
$ sudo ./sonos_pi3_start.sh 

# Control-C to exit, this takes about 10 seconds as it does a clean up before exiting
```

Now lets set the sonos_pi3_controller up to start at boot.

In your browser of choice go to the piCorePlayer admin web page ( http://[piCorePlayer Ip address]/cgi-bin/tweaks.cgi )
Scroll down the bottom and add the following to USER commands

![](https://github.com/kirkden/sonos_pi3_refresh/blob/main/sonos_pi3_controller/images/piCorePlayer%20User%20Commands.png)

sudo /home/tc/sonos_pi3_controller/sonos_pi3_start.sh

```
Hit Save and backup sonos_pi3_controller config
```

```
$ pcp bu
$ pcp reboot
```

#### Step 5 - Serial Console

I decided to set up a serial console to a 3.5mm audio jack so I didnt have to dismantle the Sonos pi:3 if something went wrong.  That said, I have never used it.
        
**<u>Note:</u>**
Its 3.3V so you need an adaptor. I used an XBEE adapter (similar to https://www.dfrobot.com/product-72.html)
        

```
$ m1
$ cd /mnt/mmcblk0p1
$ sudo vi cmdline.txt

# and append the following to cmdline.txt

console=serial0,115200 earlycon=uart8250,mmio32,0x3f215040

$ sudo vi config.txt
# and ensure enable_uart=1 is set
```

Your all finished the software set up phase, final steps are:

```
$ pcp bu
$ pcp reboot   
```

 And if all works as expected, move on the assembling the sonos pi:3 case

### Part B: Hardware

#### Step 1 - List of components

- 3D printed mounting bracket ([Fusion360 File](https://github.com/kirkden/sonos_pi3_refresh/blob/main/sonos_pi3_hardware/Sonos%20Play3%20Pi%20mounting%20tray%20v18.f3d))

- Sonos Play:3 Gen 1
- Raspberry Pi 3 Model A+ (https://core-electronics.com.au/raspberry-pi-3-model-a-plus.html)
- HiFiBerry Amp2+ (https://www.hifiberry.com/shop/boards/hifiberry-amp2/)
- Optional:
  3.5mm audio socket (https://www.jaycar.com.au/3-5mm-enclosed-socket/p/PS0122)
  USB Keystone insert (https://www.jaycar.com.au/keystone-insert-usb-usb-skt/p/PS0773)
  Short USB A male to male cable (https://www.jaycar.com.au/0-5m-usb-2-0-a-male-to-a-male-cable/p/WC7707)
- Assorted screws, nuts and bolts, cable ties

#### Step 2 - Dismantling

1. Watch this youtube [What's Inside?-Sonos PLAY:3 Teardown](https://www.youtube.com/watch?v=GyehWjqiE-Y&feature=youtu.be) also https://www.ifixit.com/Teardown/Sonos+Play:3+Teardown/12475 is a good guide as well
2. Remove to main board and put it aside, you wont be needing it any more.
3. Remove the push button daughter board
4. Leave the power supply in place you will use that to power the RaspberryPi and Amp

#### Step 3 - Assembling

1. 3D print [mounting frame STL](https://github.com/kirkden/sonos_pi3_refresh/blob/main/sonos_pi3_hardware/Sonos%20Play3%20Pi%20mounting%20tray%20v18.stl) - PLA or ABS is fine, I have included the Fusion 360 files if you need to modify for your requirements
   ![](https://github.com/kirkden/sonos_pi3_refresh/blob/main/sonos_pi3_hardware/Sonos%20Play3%20Pi%20mounting%20tray%20v18.png)
   
2. Mount Raspberry Pi and Amp2+ either side of the freshly printed mounting frame
   ![](https://github.com/kirkden/sonos_pi3_refresh/blob/main/sonos_pi3_hardware/images/pi%20amp2%20and%20USB%20mount%201.jpg)
   
3. Remove daughter board cable plug and solder on wires per Fritzing diagram ![Button and LED wiring](https://github.com/kirkden/sonos_pi3_refresh/blob/main/sonos_pi3_hardware/images/Sonos%20button%20panel%20wiring%202.jpg)

   Wiring diagram for Pause/Volume Push Buttons ![](https://github.com/kirkden/sonos_pi3_refresh/blob/main/sonos_pi3_hardware/images/Sonos_pi3_wiring_schem%20-%202020-12-10T215039.420302Z.jpg)

4. Cut off connector on power supply cable connector and connect wires,    Warning I found both +ve and -ve 15v on the power supply so measure with a multi-meter to find +15v and connect to Amp2.  The HiFiBerry powers the Rasp Pi so thats nicely looked after.

5. Speaker connection

   a) Connect the two drivers in series 

   ![](https://github.com/kirkden/sonos_pi3_refresh/blob/main/sonos_pi3_hardware/images/Speaker%20wiring%20diagram.png)

   ![](https://github.com/kirkden/sonos_pi3_refresh/blob/main/sonos_pi3_hardware/images/Speaker%20Driver%20wiring.jpg)

6. USB 
   Insert the Keystone USB connector and
   ![](https://github.com/kirkden/sonos_pi3_refresh/blob/main/sonos_pi3_hardware/images/pi%20amp2%20and%20USB%20mount%201.jpg)

7. Serial Port
   If you want to connect a serial port, I used a 3.5mm socket mounted in the back of the Sonos case, on reflection, I would chose a different connector type as it looks like an audio in/out feature. 
   It is connected to pins 8 (TX) and 10 (RX).  Note:  As previously mentioned its 3.3v so used a suitable TTL converter

   ![](https://github.com/kirkden/sonos_pi3_refresh/blob/main/sonos_pi3_hardware/images/Serial%20Socket%20mount.jpg)

8. Hot glue all connections

   

## Section: 3 Creating Room Equalization

This step is probably optional, you could just use the standard EQ in piCorePlayer and set to your liking, but I really wanted to have room EQ setup all my speakers to a standard "house curve" then I tweak from there.  

Before starting I might help to understand the flow I used.  I installed a microphone, REW and the APO Equalizer on a Win10 PC.   The PC was connected to the sonos:pi3 via Bluetooth.  That way it was easy to make changes to the room curve and remeasure results.

**Important**:  Dont do this in the middle of the night, its quite loud

#### Step 1: Room EQ wizard

- Download and install REW from here https://www.roomeqwizard.com/
- I strongly suggest you watch this youtube first, its really helpful and understanding how to get the best results:  https://www.youtube.com/watch?v=Ev1bSSL8tRA
- I used this type of microphone https://www.daytonaudio.com/product/1052/omnimic-v2-precision-measurement-system

#### Step 2: APO

The APO EQ was chosen because you can export EQ curves from REW and important than directly into APO, tweat and test again.

- Download and install APO EQ from here https://equalizerapo.com/
- I found it not as intuitive so I suggest have a play with it first with some headphones and watch the youtube  https://www.youtube.com/watch?v=zs2tdnF9omw&feature=youtu.be so you understand how to enable and review curves from REW.

#### Step 3: Deploy EQ curve 

OK, now for the manual part.  For this step we will use the default files located in https://github.com/kirkden/sonos_pi3_refresh/tree/main/sonos_pi3_crossover/dsp_crossover_config.

Once you have a txt file of the desired EQ settings from APO you need to convert them into a format compatible with the dsp plugin, luckily we have script (rew_to_dsp.sh) provided by Michael Barbour with his DSP plugin https://github.com/bmc0 to make this easier.

Check the format of the APO EQ output file ( Bass_Limited_filters_75db.txt ) and check that it looks something like below 

```
$ cat Bass_Limited_filters_75db.txt 

Room EQ V5.19
Dated: 02/10/2020 8:55:12 AM

Notes:

Equaliser: Generic
Average 1
Filter  1: ON  PK       Fc    5818 Hz  Gain  -4.7 dB  Q 1.000
Filter  2: ON  PK       Fc    1229 Hz  Gain  -4.5 dB  Q 1.454
Filter  3: ON  PK       Fc    1380 Hz  Gain  -4.0 dB  Q 1.412
Filter  4: ON  PK       Fc    150 Hz   Gain   8.0 dB  Q 1.000
```

```
login as tc/piCore

$ m2
$ cd /mnt/mmcblk0p2/tce/sonos_pi3_refresh/sonos_pi3_crossover/dsp_crossover_config

$ sh ./rew_to_dsp.sh Bass_Limited_filters_75db.txt > Bass_Limited_filters_75db.config

# Check the config file and make sure it looks like below.  You may need to manually edit the source or destination a little to get it right. 

$ cat Bass_Limited_filters_75db.config
```

```
effects_chain=eq 5818 1.000 -4.7 eq 1229 1.454 -4.5 eq 1380 1.412 -4.0 eq 150 1.000 8.0
```

- The next part installs the EQ curve.   Earlier in the install  process we installed a default EQ now install your custom one and reboot.

```
$ sudo install -m644 -D Bass_Limited_filters_75db.config /etc/ladspa_dsp/config
$ pcp bu
$ pcp reboot
```

- Test and Done - Hope you get many hours of enjoyment

Thanks again to all the contributors to open source and public domain software.



------

## References

### Helpful Links 

Big thanks to all the individuals and teams for the dedication and effort they have gone too in developing and maintaining  great software, tools and sharing information.

​	https://alsa.opensrc.org/Ladspa_(plugin)
​	https://github.com/iem-projects/alsa-ladspa-bridge
​	http://plugin.org.uk/ladspa-swh/docs/ladspa-swh.html
​	https://rtaylor.sites.tru.ca/2013/06/25/digital-crossovereq-with-open-source-software-howto/
​	https://www.youtube.com/watch?v=Ev1bSSL8tRA    ### REW Room EQ
​	https://github.com/bmc0/dsp ### DSP framework end for EQ filters
​	https://github.com/bmc0/dsp/wiki/System-Wide-DSP-Guide ### DSP setup example
​	http://quitte.de/dsp/caps.html ### CAPS plugins
​	http://[LMS Server IP address]:9000/html/docs/cli-api.html?player=  ### List of LMS API commands

### piCorePlayer useful commands

| PCP Alias/cmd                                             |                                                 |
| --------------------------------------------------------- | ----------------------------------------------- |
| ce                                                        | Change directory to /mnt/mmcblk0p2/tce          |
| ceo                                                       | Change directory to /mnt/mmcblk0p2/tce/optional |
| m1                                                        | Mount the boot partition /mnt/mmcblk0p1         |
| m2                                                        | Mount the second partition /mnt/mmcblk0p2       |
| c1                                                        | Change directory to /mnt/mmcblk0p1              |
| c2                                                        | Change directory to /mnt/mmcblk0p2              |
| vicfg                                                     | Edit configuration file config.txt using vi     |
| vicmd                                                     | Edit boot file cmdline.txt using vi             |
| u1                                                        | Unmount the boot partition /mnt/mmcblk0p1       |
| u2                                                        | Unmount the second partition /mnt/mmcblk0p2     |
| [pcp](https://docs.picoreplayer.org/information/pcp_cli/) | piCorePlayer Command Line Interface (CLI)       |

| Audio oriented commands                     |                                                              |
| ------------------------------------------- | ------------------------------------------------------------ |
| analyseplugin /usr/lib/ladspa/RTallpass1.so | Provide details of plugin eg $ analyseplugin /usr/lib/ladspa/RTallpass1.so |
| listplugins                                 | List installed plugins                                       |
| dsp                                         | An audio processing program with an interactive mode         |
| speaker-test                                | Play audio                                                   |



### LADSP Installed Effects list

| [DSP](https://github.com/bmc0/dsp) (Michael Barbour)         |
| ------------------------------------------------------------ |
| Sine Oscillator (Freq:audio, Amp:audio) (1044/sine_faaa)     |
| Sine Oscillator (Freq:audio, Amp:control) (1045/sine_faac)   |
| Sine Oscillator (Freq:control, Amp:audio) (1046/sine_fcaa)   |
| Sine Oscillator (Freq:control, Amp:control) (1047/sine_fcac) |
| White Noise Source (1050/noise_white)                        |
| ladspa_dsp (2378/ladspa_dsp)                                 |
| Simple Low Pass Filter (1041/lpf)                            |
| Simple High Pass Filter (1042/hpf)                           |
| Simple Delay Line (1043/delay_5s)                            |
|                                                              |
| [**CAPS**](http://quitte.de/dsp/caps.html)  **(Tim Goetze)** the C* Audio Plugin Suite |
| C* Noisegate - Attenuating hum and noise (2602/Noisegate)    |
| C* Compress - Compressor and saturating limiter (1772/Compress) |
| C* CompressX2 - Stereo compressor and saturating limiter (2598/CompressX2) |
| C* ToneStack - Classic amplifier tone stack emulation (2589/ToneStack) |
| C* AmpVTS - Idealised guitar amplification (2592/AmpVTS)     |
| C* CabinetIII - Simplistic loudspeaker cabinet emulation (2601/CabinetIII) |
| C* CabinetIV - Idealised loudspeaker cabinet (2606/CabinetIV) |
| C* Plate - Versatile plate reverb (1779/Plate)               |
| C* PlateX2 - Versatile plate reverb, stereo inputs (1795/PlateX2) |
| C* Saturate - Various static nonlinearities, 8x oversampled (1771/Saturate) |
| C* Spice - Not an exciter (2603/Spice)                       |
| C* SpiceX2 - Not an exciter either (2607/SpiceX2)            |
| C* ChorusI - Mono chorus/flanger (1767/ChorusI)              |
| C* PhaserII - Mono phaser (2586/PhaserII)                    |
| C* AutoFilter - Self-modulating resonant filter (2593/AutoFilter) |
| C* Scape - Stereo delay with chromatic resonances (2588/Scape) |
| C* Eq10 - 10-band equaliser (1773/Eq10)                      |
| C* Eq10X2 - Stereo 10-band equaliser (2594/Eq10X2)           |
| C* Eq4p - 4-band parametric shelving equaliser (2608/Eq4p)   |
| C* EqFA4p - 4-band parametric eq (2609/EqFA4p)               |
| C* Wider - Stereo image synthesis (1788/Wider)               |
| C* Narrower - Stereo image width reduction (2595/Narrower)   |
| C* Sin - Sine wave generator (1781/Sin)                      |
| C* White - Noise generator (1785/White)                      |
| C* Fractal - Audio stream from deterministic chaos (1774/Fractal) |
| C* Click - Metronome (1769/Click)                            |
| C* CEO - Chief Executive Oscillator (1770/CEO)               |
|                                                              |
| **[RT-Plugins](https://faculty.tru.ca/rtaylor/rt-plugins/index.html)** **(Richard Taylor)** |
| Mono Amplifier (1048/amp_mono)                               |
| Stereo Amplifier (1049/amp_stereo)                           |
| RT parametric eq (9001/RTparaeq)                             |
| RT LR4 lowpass (9020/RTlr4lowpass)                           |
| RT LR4 highpass (9021/RTlr4hipass)                           |
| RT Low Shelf (9002/RTlowshelf)                               |
| RT First Order Lowpass (9015/RTlowpass1)                     |
| RT 2nd-order lowpass (9006/RTlowpass)                        |
| RT High Shelf (9003/RThighshelf)                             |
| RT First Order Highpass (9016/RThighpass1)                   |
| RT 2nd-order highpass (9007/RThighpass)                      |
| RT mls-based stereo decorrelation (9011/RTdecorrmls)         |
| RT 2nd-Order Allpass (9005/RTallpass2)                       |
| RT First Order Allpass (9010/RTallpass1                      |