# -*- coding: utf-8 -*-
# --------------------------------------------------------------------------------- 
#
# mqtt_controller
#       - Intended to pause player when an MQTT message arrives.
#         Payload is undefined.   Intended to be used with my doorbell
#
# --------------------------------------------------------------------------------- 

import  asyncio
import  config

from    gmqtt                import Client as MQTTClient
from    gmqtt.mqtt.constants import UNLIMITED_RECONNECTS

import  logging
logger  = logging.getLogger(__name__)

# --------------------------------------------------------------------------------- 
def on_connect(client, flags, rc, properties):
    logger.info('MQTT Broker Connected')
    client.subscribe(f'{config.args.mqtt_topic}', qos=1)

# --------------------------------------------------------------------------------- 
#
# on_message()
#       - When a valid MQTT message arrives, send a pause to the player
#         only if its playing, otherwise do nothing
#
# --------------------------------------------------------------------------------- 
async def on_message(client, topic, payload, qos, properties):
    logger.debug(f'MQTT RECV: {topic}, {payload.decode("utf-8")}')

    try:
        _lms = client.properties['lms_controller'] # Get lms class object

        _mode = _lms.get_player_mode()

        if _mode == 'play':
            _lms.send_lms_command('pause')

    except Exception as e:
        logger.warning(f'MQTT on_message failed - {payload.decode("utf-8")} - {e}')

    pass
    return 0

def on_disconnect(client, packet, exc=None):
    logger.info('MQTT Disconnected')
    

def on_subscribe(client, mid, qos, properties):
    logger.debug('MQTT Subscribed')

# --------------------------------------------------------------------------------- 
#
# mqtt_monitor()
#       - Connect to configured MQTT server
#       - Configure auto-reconnect
#       - The loop does nothing just yields to asyncio loop
#
# --------------------------------------------------------------------------------- 

async def mqtt_monitor(_lms):

    _client = MQTTClient(f'{config.args.player_name}.squeeze', lms_controller=_lms)

    _client.on_connect    = on_connect
    _client.on_message    = on_message
    _client.on_disconnect = on_disconnect
    _client.on_subscribe  = on_subscribe

    _client.set_config({
        'reconnect_retries': UNLIMITED_RECONNECTS,
        'reconnect_delay': 60
    })

    _client.set_auth_credentials(config.args.mqtt_user, password=config.args.mqtt_pass)  
   
    try:
        await _client.connect(host=config.args.mqtt_ip, port=config.args.mqtt_port)

    except Exception as e:
    
        logger.error(f'MQTT Client connect failed {config.args.mqtt_ip}:{config.args.mqtt_port} - {e}')
        return False
    
    else:

        while True:
            await asyncio.sleep(10)
    
    finally:
        await _client.disconnect()



