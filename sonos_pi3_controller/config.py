# -*- coding: utf-8 -*-
# ---------------------------------------------------------------------------------
#
# config
#     - Configure required setting for tge application
#     - These can get set in both or either of a config file or the command line
#     - command line arguments overide config file 
#
# ---------------------------------------------------------------------------------
import socket
import re
import logging 

logging.basicConfig(
   level=logging.DEBUG, 
   format='%(asctime)s %(funcName)s [%(levelname)s] %(message)s',
   datefmt='%d %b %H:%M:%S')

logger = logging.getLogger(__name__)
logger.info('Starting sonos_pi3_controller')

try:
   import configargparse
except:
   logger.error(f'configargparse module not installed correctly, see install notes')
   exit(1)

# ---------------------------------------------------------------------------------
#
# Validation functions
#     - GPIO pins
#     - IPv4 address
#     - MQTT Topic syntax
# ---------------------------------------------------------------------------------
valid_GPIO_PINS=[7,9,11,12,13,15,16,18,19,21,22,23,24,26,27,28,29,31,32,33,35,36,37,38,40]

class is_ipv4_address_valid(configargparse.Action):   
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, values)
        try:
             _ip = socket.inet_pton(socket.AF_INET, socket.gethostbyname(values))
        except:
            raise Exception(f'Invalid IPV4 address in config - {values}')
            

class is_topic_valid(configargparse.Action):   
    def __call__(self, parser, namespace, values, option_string=None):
         setattr(namespace, self.dest, values)
         if re.match(r'[^/]([\S\w]+/)+', values) == None:
               raise Exception (f'MQTT topic syntax error in Config - {values}')


# ---------------------------------------------------------------------------------
#
# Parse config file/command line arguments
#
# ---------------------------------------------------------------------------------
def parse_args():


   p = configargparse.ArgParser(default_config_files=['/mnt/OneDrive_ckirkby/Source/Raspi/sonos_pi3_controller/config.ini','/home/tc/sonos_pi3_controller/config.ini', '~/.config.ini'])
   p.add('--config', is_config_file=True, help='config file path')
   p.add('--log_level', type=str, choices=['debug', 'info', 'warning', 'error', 'critical'], default='error')
   p.add('--backup_cmd', help='on piCorePlayer a backup is required for pairing to survive reboot usually "/usr/local/bin/pcp, bu"', action="append")

   p.add('--led1', type=int, help='pin for volume activity LED', choices=valid_GPIO_PINS, required=True)
   p.add('--led2', type=int, help='pin for the play and pause LED', choices=valid_GPIO_PINS, required=True)
   p.add('--led3', type=int, help='pin for the error LED', choices=valid_GPIO_PINS, required=True)
   p.add('--led4', type=int, help='pin for the warning LED', choices=valid_GPIO_PINS, required=True)
  
   p.add('--check_ip', type=str, help='IPv4 Address of host for network check', default= '', action=is_ipv4_address_valid) 
   p.add('--check_port', type=int, help='Port number for network check', default=80)
   
   p.add('--lms_ip', type=str, help='LMS Server IPv4 Address', required=True, action=is_ipv4_address_valid)
   p.add('--lms_port', type=int, help='LMS Serever port number usually 9090', default=9090) 
   
   p.add('--player_name', type=str, help='Squeezelite player name', required=True)
   p.add('--vol_step', type=int, help='Volume +/- step size', choices=[1,2,5,10], default=5)

   p.add('--mute', type=int, help='Hardware Mute pin (ex pin 7 on HiFiBerry Amp2+', default=0)
      
   p.add('--play_pause', type=int, help='GPIO pin for the play and pause button', choices=valid_GPIO_PINS, required=True)
   p.add('--vol_up', type=int, help='GPIO pin for volume+', choices=valid_GPIO_PINS, required=True)
   p.add('--vol_down', type=int, help='GPIO pin for volume-', choices=valid_GPIO_PINS, required=True)
   p.add('--long_press', type=int, help='Long button press duration in msecs', default=1200)
  
   p.add('--bluetoothctl_path', help='path to the bluetoothctl program', default='')
   p.add('--bt_timeout', help='Bluetooth discoverale timeout in secs', default='120')
   p.add('--bt_config_path', help='path to Bluetooth config file', default='/usr/local/etc/pcp/pcp-bt.conf')
   p.add('--bt_restart_cmd', help='Bluetooth speaker daemon restart cmd usually "/usr/local/etc/init.d/pcp-bt6 restart"', action="append")

   p.add('--mqtt_ip', help='IPv4 address of mqtt broker', default='', action=is_ipv4_address_valid)
   p.add('--mqtt_port', help='mqtt port usually 1883', default='1883') 
   p.add('--mqtt_user', help='mqtt username', default='')
   p.add('--mqtt_pass', help='mqtt password', default='')
   p.add('--mqtt_topic', help='mqtt subscibe topic, used to pause player', default='squeezelite/pause', action=is_topic_valid)

   global args
   try:
      args = p.parse_args()
   except Exception as e:
      logger.error(f'configarfparse error - {e}')
      exit(1)

   #--------------------
   # Set Log level
   log_level = getattr(logging, args.log_level.upper())

   if not isinstance(log_level, int):
      raise ValueError('ERROR Invalid log level: %s' % log_level)
   
   logging.getLogger().setLevel(log_level)

   #--------------------
   # Print user configuration
   logger.debug(f'For command help see http://{args.lms_ip}:9000/html/docs/cli-api.html?player=\n\n')     
   logger.info(p.format_values())
