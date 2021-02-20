# -*- coding: utf-8 -*-
# --------------------------------------------------------------------------------- 
#
# gpio_controller
#       get_press_duration()
#       set_short_press_functions()
#       set_long_press_functions()
#       set_led_state()
#       get_led_state()
#       close()
#       led_monitor()
#       button_monitor()
#
# --------------------------------------------------------------------------------- 

try:
    import RPi.GPIO as GPIO
except RuntimeError:
    print("Error importing RPi.GPIO!  This is probably because you need superuser privileges.  You can achieve this by using 'sudo' to run your script")

import  asyncio
import  config
import  logging

from    time import sleep

from    bt_pair_controller import set_bt_pairing

logger  = logging.getLogger(__name__)
_flash  = {}

class gpio_Controller:

    # --------------------------------------------------------------------------------- 
    #
    # GPIO Pin initialisation
    #
    # ---------------------------------------------------------------------------------
    def __init__(self, lms):

        logger.info('Initialising GPIO Controller')

        self.lms = lms

        GPIO.setmode(GPIO.BOARD)
        GPIO.setwarnings(False)

        # Setup button pins
        GPIO.setup(config.args.play_pause, GPIO.IN, pull_up_down=GPIO.PUD_UP)
        GPIO.setup(config.args.vol_down, GPIO.IN, pull_up_down=GPIO.PUD_UP)
        GPIO.setup(config.args.vol_up, GPIO.IN, pull_up_down=GPIO.PUD_UP)
        
        # Setup LEDs
        led_list = [config.args.led1, config.args.led2, config.args.led3, config.args.led4]           
        GPIO.setup(led_list, GPIO.OUT, initial=GPIO.LOW)

        _flash[config.args.led1] = False
        _flash[config.args.led2] = False
        _flash[config.args.led3] = False
        _flash[config.args.led4] = False

        # HiFiBerry Amp2 LOW = Mute
        if config.args.mute > 0:
            GPIO.setup(config.args.mute, GPIO.OUT, initial=GPIO.HIGH)

    #-------------------------------------------------------------------------------------
    #
    # set_short_press_functions():   
    #       - Send play or pause command to the LMS server
    #       - If configured set the Hard mute (HiFIBerry amp2+) ON/OFF
    #       - Send Volume +/- commands
    #       - Set the state of the LED's
    #  
    #-------------------------------------------------------------------------------------
    def set_short_press_functions(self, _button):

        _command = ''
        _mode = self.lms.get_player_mode()

        logger.debug(f'Short press({_button}) Mute pin({config.args.mute}) Player({_mode})')

        # Toggle playing mode        
        if _button == config.args.play_pause:

            if _mode == 'play':
                gpio_Controller.set_hardware_mute(self, 'ON')      
                gpio_Controller.set_led_playing_status(self, 'pause')
                _command = 'pause'
            
            if _mode == 'pause' or _mode == 'stop':
                gpio_Controller.set_hardware_mute(self, 'OFF') 
                gpio_Controller.set_led_playing_status(self, 'play')
                _command = 'play'

            if _mode == 'error':        # If LMS server is unavailable still do hardware mute
                gpio_Controller.set_hardware_mute(self, 'TOGGLE')
        
        # Adjust volume
        if _button == config.args.vol_down:
            gpio_Controller.set_led_state(self, config.args.led1, 'BLINK')
            _command = f'mixer volume -{config.args.vol_step}'
        
        if _button == config.args.vol_up:
            gpio_Controller.set_led_state(self, config.args.led1, 'BLINK')
            _command = f'mixer volume +{config.args.vol_step}'

        self.lms.send_lms_command(_command)


    #-------------------------------------------------------------------------------------
    #
    # set_long_press_functions():   
    #       - Set alternate functions for a long press of the buttons
    #       - play/pause == Bluetooth pairing
    #       - Volume + == Jump forward a track
    #       - Volume - == Jump back a track
    #  
    #-------------------------------------------------------------------------------------
    async def set_long_press_functions(self, _button):
    
        _command = ''
        _mode = self.lms.get_player_mode()

        logger.debug(f'Long press({_button}) Mute pin({config.args.mute}) Player({_mode})')

        # Play/Pause button start Bluetooth paring mode if path is supplied
        if _button == config.args.play_pause:

            if config.args.bluetoothctl_path != '':

                logger.info('*** Bluetooth Pairing mode')

                self.lms.send_lms_command('stop')
            
                gpio_Controller.set_led_state(self, config.args.led4, 'FLASH')
        
                await set_bt_pairing()
                
                # Pairing complete turn off flash
                gpio_Controller.set_led_state(self, config.args.led4, 'OFF')

                logger.info('*** Bluetooth Pairing mode finished')

        # Use volume buttoms for jump forward/back
        elif _button == config.args.vol_down:
            gpio_Controller.set_led_state(self, config.args.led1, 'BLINK')
            _command = 'button jump_rew'
        
        elif _button == config.args.vol_up:
            gpio_Controller.set_led_state(self, config.args.led1, 'BLINK')
            _command = 'button jump_fwd'

        if _command != '':
            self.lms.send_lms_command(_command)
    
    # --------------------------------------------------------------------------------- 
    #
    # set_led_state()
    #       - Set the state of specific LED
    #       - Flashing leds is handled asynchronously in the led_monitor loop.
    #
    # ---------------------------------------------------------------------------------
    def set_led_state(self, _led, _state):
        
        logger.debug(f'LED ({_led}) : {_state}')
        
        # Need to turn off flash in case we are transitioning from flash to off/on etc
        _flash[_led] = False

        if _state == 'FLASH':
            _flash[_led] = True
        elif _state == 'TOGGLE':
            GPIO.output(_led, not GPIO.input(_led))
        elif _state == 'ON':  
            GPIO.output(_led, GPIO.HIGH)
        elif _state == 'OFF':
            GPIO.output(_led, GPIO.LOW)
        elif _state == 'BLINK':
            GPIO.output(_led, GPIO.HIGH)
            sleep(0.5)
            GPIO.output(_led, GPIO.LOW)

    # --------------------------------------------------------------------------------- 
    # Return PIN state.  
    # if the led is flashing return HIGH to indicate its set ON

    def get_led_state(self, _led):
        
        if _flash[_led] != False:
            return GPIO.HIGH
        else:
            return( GPIO.input(_led) )


    # --------------------------------------------------------------------------------- 
    #
    # close()
    #       - Graceful cleanup
    #
    # ---------------------------------------------------------------------------------
    def close(self):

        logger.info('GPIO Clean close requested')
        GPIO.cleanup()

    # ---------------------------------------------------------------------------------
    #
    # led_monitor()
    #       - Run from main(), used to monitor change in player state either triggered
    #         from the physical buttons or other applications.
    #
    # ---------------------------------------------------------------------------------
    async def led_monitor(self):

        _last_mode = None

        while True:

            _mode = self.lms.get_player_mode()

            if _mode != _last_mode:
                gpio_Controller.set_led_playing_status(self, _mode)
                _last_mode = _mode

            # Flashing LED's are managed here.
            for _led in _flash:
                if _flash[_led]:
                    GPIO.output(_led, not GPIO.input(_led))

            try:

                await asyncio.sleep(0.5)    
            
            except Exception as e:
                raise Exception(f'led_monitor(): asyncio.sleep failed - {e}')
            
    # ---------------------------------------------------------------------------------
    #
    # set_led_player_status()
    #       - Change the LEDs based on playing mode
    #       - Set Hardware mute if configured
    #
    # ---------------------------------------------------------------------------------
    def set_led_playing_status(self, _mode):

        if _mode == 'play':       
            gpio_Controller.set_hardware_mute(self, 'OFF')   
            gpio_Controller.set_led_state(self, config.args.led2, 'OFF')
        
        if _mode == 'pause':
            gpio_Controller.set_hardware_mute(self, 'ON')   
            gpio_Controller.set_led_state(self, config.args.led2, 'ON')

        if _mode == 'stop':
            gpio_Controller.set_led_state(self, config.args.led2, 'OFF')

    # ---------------------------------------------------------------------------------
    #
    # set_hardware_mute()
    #       - Set if configured.  This was set up for HifiBerry Amp2
    #
    # ---------------------------------------------------------------------------------      
    def set_hardware_mute(self, _state):

        if config.args.mute > 0:
            logger.debug(f'hardware MUTE set state to - {_state}')
            if _state == 'ON':
                GPIO.output(config.args.mute, GPIO.LOW) # Mute
            elif _state == 'OFF':
                GPIO.output(config.args.mute, GPIO.HIGH) # No Mute
            elif _state == 'TOGGLE':
                GPIO.output(config.args.mute, not GPIO.input(config.args.mute))
            else:
                logger.warning(f'hardware mute request invalid - {_state}')

    # ------------------------------------------------------------------------------------
    #
    # get_press_duration() 
    #       - Return whether a short or long button press has occured
    #       - Long press duration is set in the config file or command line
    #
    #-------------------------------------------------------------------------------------
    def get_press_duration(self,_button):
        
        i = 0
        while True:
            if ( GPIO.input(_button) == GPIO.LOW ):  # button is pressed    

                if i >= config.args.long_press:      
                    return config.args.long_press    # Long press detected    
                else:
                    i = i + 100
                    sleep(0.1) # 100 msec delay for long press timer          
            
            else:
                
                return 0                             # Short press detected
    
    # ------------------------------------------------------------------------------------
    #
    #  button_monitor()     
    #       - Called from main() program to monitor play/pause button presses
    #       - Idenitfy short and long presses and execute specified LMS functions
    #       - Using polling as threads and asyncio dont play easily
    #
    #-------------------------------------------------------------------------------------
    async def button_monitor(self):

        while True:

            _button = None

            if GPIO.input(config.args.play_pause) == GPIO.LOW:
                _button = config.args.play_pause

            if GPIO.input(config.args.vol_down) == GPIO.LOW:
                _button = config.args.vol_down

            if GPIO.input(config.args.vol_up) == GPIO.LOW:
                _button = config.args.vol_up


            # If a button has been pressed
            if _button is not None:
                if gpio_Controller.get_press_duration(self, _button) == config.args.long_press:

                    await gpio_Controller.set_long_press_functions(self, _button)           
                else:

                    gpio_Controller.set_short_press_functions(self, _button)
                
        
            try: 
                
              await asyncio.sleep(0.1)  # Influences sensitity of recognising a putton press

            except Exception as e: 
                raise Exception(f'button_monitor(): asyncio.sleep failed - {e}')
           