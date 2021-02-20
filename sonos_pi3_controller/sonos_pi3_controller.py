#!/usr/bin/python3

# --------------------------------------------------------------------------------- 
#
# Application:  sonos_pi3_controller
#     - Author:           Chris Kirkby
#     - Initial release:  28-Oct-2020
#     - Hardware Config:  Raspberry Pi 3A, HiFiberry Amp2 + in a Sonos Play:3 case
#     - Software Config:  piCorePlayer, python3
#     - Features:
#         Button control of play, pause, volume +/-, Jump forward/back
#         Reflects changes made from either buttons or applications on LEDs
#         LEDs indicate pause, play, volume +/-, network error, LMS server error
#         Short and Long Press functions (hard coded)
#         Uses hardware mute if configured
#         Works with sync players (pause will effect all)
#         Uses asyncio to improve response times
#         Bluetooth pairing button
#         mqtt subscribe - mute for doorbell
#
# --------------------------------------------------------------------------------- 

import  logging
import  config
import  asyncio
import  signal
import  functools

from    lms_Controller  import lms_Controller
from    gpio_controller import gpio_Controller
from    network_check   import network_check
from    mqtt_controller import mqtt_monitor
from    time            import sleep

logger = logging.getLogger(__name__)

# ------------------------------------------------------------------------------------- 
# Exit apllication handlers
def app_exit(signame, loop):
    logger.info(f'Exiting on signal {signame}')
    loop.stop()
    exit(0)

def error_handler(loop, context):
    logger.error('Global Exception handler called')
    loop.stop()
    exit(1)

# ------------------------------------------------------------------------------------- 
# Main application
def main(): 

      config.parse_args()   # Get user configuration from command line or file and valid

      lp = asyncio.get_event_loop()  

      for signame in {'SIGINT', 'SIGTERM'}: # Catch signals for an orderly  exit
          lp.add_signal_handler(
              getattr(signal, signame),
              functools.partial(app_exit, signame, lp))

      lp.set_exception_handler(error_handler)

      # --------------------------------------------------------------------------------- 
      # Setup logitech media server connection and GPIO pins
      #
      lms = lms_Controller()    
      gpio = gpio_Controller(lms)
    
      # --------------------------------------------------------------------------------- 
      # Start monitors
      #   - lms_monitor checks for notifications from the LMS server 
      #   - led_monitor checks the player status and sets the LEDs accordingly
      #   - button_monitor polls configured pins and executes player functions (pause, play etc)
      #
      if lp.create_task(lms.lms_monitor(gpio)) == False:
        logger.error(f'lms_monitor exited unexpectantly')
        exit(1)

      if lp.create_task(gpio.led_monitor()) == False:
        logger.error(f'gpio_led_monitor exited unexpectantly')
        exit(1)

      if lp.create_task(gpio.button_monitor()) == False:
        logger.error(f'gpio_button_monitor exited unexpectantly')
        exit(1)

      # --------------------------------------------------------------------------------- 
      # Start if configured
      #   - network_check loops doing a socket connect to configured host to check that the network is functioning
      #   - mqtt_monitor subscribes to user configured topic and pauses the player.  I use this for the doorbell
      #
      if config.args.check_ip != '':
          lp.create_task(network_check(gpio))

      if config.args.mqtt_ip != '':
          lp.create_task(mqtt_monitor(lms))
    
      
      # --------------------------------------------------------------------------------- 
      # Schedule tasks
      try:
          lp.run_forever()
      finally:
          for i in range(10):
            gpio.set_led_state(config.args.led3, 'TOGGLE')     # Visual indication we are shutting down
            sleep(0.5)

          gpio.close()
          lp.stop()
          exit(0)

if __name__ == "__main__":
    main()