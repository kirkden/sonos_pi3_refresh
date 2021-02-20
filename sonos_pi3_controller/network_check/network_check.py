# -*- coding: utf-8 -*-

from    random import seed, randint
import  config
import  asyncio

import  logging 
logger = logging.getLogger(__name__) 


# --------------------------------------------------------------------------------- 
#
# Check internet connection.
#
# --------------------------------------------------------------------------------- 

async def network_check(gpio):

    while True:
         
        logger.debug(f'Network connection test starting - {config.args.check_ip}:{config.args.check_port}')
        
        r, w = await host_connect(gpio, config.args.check_ip, 
                config.args.check_port,
                _retry_delay_min=30, _retry_delay_max=120,
                _led=config.args.led3)

        w.close()

# --------------------------------------------------------------------------------- 
#
# host_connect()
#       - Open asyncio network connection, return read/write pair
#       - Flash LED if error
#       - Random delay retry (default 10-60 seconds) if temporary network failure like
#         network timeout, host down and no route to host
#
# --------------------------------------------------------------------------------- 

async def host_connect(gpio, _ip, _port, _retry_delay_min=2, _retry_delay_max=10, _led=None):
    
    seed(1)

    while True:
        
        try:
            logger.debug(f'Connect to host - {_ip}:{_port}')

            r, w = await asyncio.open_connection(_ip, _port)
            
            if _led and gpio.get_led_state(_led):   # Connection success
                gpio.set_led_state(_led, 'OFF')

            return r, w
        
        except OSError as e:  
            gpio.set_led_state(_led, 'FLASH')
            
            # Capture standard host connection errors and retry otherwise if its more serious exit
            
            if  e.args[0] == 111 or e.args[0] == 112 or e.args[0] == 113:           
                
                logger.warning(f'Host connection failed RETRYING')       
            
            else:

                logger.error(f'Host connection error - {e}')
                raise

        finally:
            # Random delay for next connection 
            n = randint(_retry_delay_min, _retry_delay_max)
            try:
                await asyncio.sleep(n)
            except Exception as e:
                raise Exception(f'host_connect(): asyncio.sleep() failed - {e}')

            
