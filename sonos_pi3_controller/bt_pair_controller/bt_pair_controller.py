# -*- coding: utf-8 -*-
# --------------------------------------------------------------------------------- 
#
# bt_pair_controller()
#       get_bt_response()
#       set_bt_pairing()
#       backup_config()
# --------------------------------------------------------------------------------- 
import asyncio
import logging
import pexpect
import sys
import subprocess
import config

logger = logging.getLogger(__name__)

async def set_bt_pairing():

    # bluetoothctl prompt includes binary chars for colored fonts - arggg
    btc_prompt = r'\033\[\d+\;\d+m\[.+\]\033\[\d+m#\s*$'
    
    try:
        logger.info(f'Bluetoothctl path {config.args.bluetoothctl_path}')
        btc = pexpect.spawn(config.args.bluetoothctl_path, timeout=10)
    
    except Exception as e:
        logger.error(f'Bluetoothctl spawn failed - {e}')
        return False


    if config.args.log_level == 'debug':            # Enable verbose messages from pexpect
        btc.logfile = sys.stdout.buffer
    
    await get_bt_response(btc, btc_prompt)

    btc.sendline('agent on')
    await get_bt_response(btc, btc_prompt)
    
    btc.sendline(f'discoverable-timeout {config.args.bt_timeout}')
    await get_bt_response(btc, btc_prompt)

    btc.sendline('discoverable on')
    await get_bt_response(btc, btc_prompt)

    btc.sendline('pairable on')
    await get_bt_response(btc, btc_prompt)

    btc.sendline('advertise on')
    await get_bt_response(btc, btc_prompt)

    # --------------------------------------------------------------------------------- 
    #
    # Validate device and trust for both new and existing devices
    #
    # --------------------------------------------------------------------------------- 
    
    _state = 0
    _response = ' '

    try:

        # -----------------------------------------------------------------------------
        # 1. Wait for device to be connected (existing) or paired (new)

        _paired        = '(Device )(?P<devID>(?:[0-9a-fA-F]:?){12})( Paired: yes)'
        _connected     = '(Device )(?P<devID>(?:[0-9a-fA-F]:?){12})( Connected: yes)'
        
        _state, _response = await get_bt_response(btc, _paired, _connected, _timeout=60)

        if _state == 2 or _state == 3:        # Connected or paired OK
            _btDevID = btc.match.group('devID').decode().rstrip()
            
            # -------------------------------------------------------------------------
            # 2. Trust the device 

            btc.sendline(f'trust {_btDevID}')
            _trusted1 = f'(Device\s+)({_btDevID})(\s+Trusted: yes)'
            _trusted2 = f'(\s+Changing\s+)(({_btDevID}))(\s+trust succeeded)'

            _state, _response = await get_bt_response(btc, _trusted1, _trusted2)
            if _state == 2 or _state == 3:    # New or existing trust

                logger.info(f'Device trusted - {_state}')

                # ----------------------------------------------------------------------
                # 3. Get the device name 

                btc.sendline(f'info {_btDevID}')
                _name = '(\sName:\s)(?P<devName>([\x20-\x7e]+\s+?))'

                _state, _response = await get_bt_response(btc, _name, _timeout=5)
                if _state == 2:              # Found Bluetooth device name    

                    _btDevName = btc.match.group('devName').decode().rstrip()
                    logger.debug(f'Bluetooth device name - {_btDevName}')

                    # ----------------------------------------------------------------------
                    # 4. Save the config 
                      
                    save_picoreplayer_config(_btDevID, _btDevName)

    except Exception as e:
        logger.warning(f'Bluetooth pair warning - {e}')
        
    if _state < 2:
        logger.warning(f'Bluetooth pairing time out - Try again')
        
    btc.sendline('pairable off')
    btc.sendline('discoverable off')
    btc.sendline('agent off')
    btc.sendline('quit')

    if btc.isalive():
        btc.sendline('quit') #  ask btc to exit.
        btc.close()

    if btc.isalive():
        logger.info('btc did not exit gracefully.')
    else:
        logger.info('btc exited gracefully.')

#------------------------------------------------------------------------------
# 
# For piCorePlayer need change the device mode to speaker and do a backup 
# otherwise paired device wont be remembered after boot
# 
#------------------------------------------------------------------------------

def save_picoreplayer_config(_btDevID, _btDevName):

    update_bt_device_config(_btDevID, _btDevName)
    backup_config()
    restart_bluetooth()
 

# --------------------------------------------------------------------------------- 
#
# get_bt_response()
#   Wait for _response from bluetoothctl, consume timeout or unexpect EOL
#   so application doesnt crash 
#
# --------------------------------------------------------------------------------- 
async def get_bt_response(btc, *_args, _timeout=10):

    _pattern = [pexpect.EOF, pexpect.TIMEOUT]

    for s in _args:                 # Add timeout and EOF patterns
        _pattern.append(s)

    try:

        _state = await btc.expect(_pattern, timeout=_timeout, async_=True)
   
    except Exception as e:
        logger.error(f'Bluetooth btc.expect failed - {e}')
        raise Exception(f'get_bt_response() await btc.expect() failed - {e}')
    
    else:
        if _state < 2:
            return _state, 'TIMEOUT/EOF'
        else: 
            return _state, btc.match.group().decode("utf-8")
    

# --------------------------------------------------------------------------------- 
#
# backup_config()
#       -  on piCorePlayer a backup is required for pairing to survive reboot
#
# --------------------------------------------------------------------------------- 
def backup_config():

    try:
        logger.info(config.args.backup_cmd)
        subprocess.call(config.args.backup_cmd)

    except Exception as e:
        logger.error(f'Backing up BT pairing configuration error {config.args.backup_path} - {e}')

# --------------------------------------------------------------------------------- 
#
# restart_bluetooth()
#       -  on piCorePlayer need to launch bluealsa-aplay to be able to play to the speaker
#
# --------------------------------------------------------------------------------- 
def restart_bluetooth():

    try:
        logger.info(f'Restarting Bluetooth - {config.args.bt_restart_cmd}')
        subprocess.call(config.args.bt_restart_cmd)

    except Exception as e:
        logger.error(f'Error restarting Bluetooth {config.args.bt_restart_cmd} {e}')


# --------------------------------------------------------------------------------- 
#
#   update_bt_device_config()
#
#       With piCorePlayer, the mode of connected bluetooth devices is saved in
#       /usr/local/etc/pcp/pcp-bt.conf
#       
#       Format is <mac address>#<name>#<delay>#mode
#       Mode is either 1 = Speaker, 2 = Player, 3 = Streamer    
#       eg: XX:XX:XX:XX:XX:XX#pop-os#10000#2
#       
#       By default when a new device is connected as a Speaker
#     
# --------------------------------------------------------------------------------- 

import os
import sys

def update_bt_device_config(_btDevID, _btDevName):

    if os.path.exists(config.args.bt_config_path):

            try:

                with open(config.args.bt_config_path, 'r') as r:
                    buf = r.readlines()

                if not buf:
                    logger.error(f'Updating bluetooth config, No data in file - {config.args.bt_config_path}')
                    return False
                
                else:

                    _exists = False
                    for line in range(0, len(buf)):
                        bt_fld = buf[line].split('#')

                        if len(bt_fld) == 4:
                            if bt_fld[0] == _btDevID and int(bt_fld[3]) != 2: 
                                _exists = True
                                buf[line] = f'{bt_fld[0]}#{bt_fld[1]}#{bt_fld[2]}#2\n'

                            if bt_fld[0] == _btDevID and int(bt_fld[3]) == 2:
                                _exists = True

                    # No record exists, need to add one
                    if _exists == False:    
                        buf.append(f'{_btDevID}#{_btDevName}#10000#2\n')

                    with open(config.args.bt_config_path, 'w') as w:
                        w.writelines(buf)
                        logger.info(f'Successfully updated Bluetooth device mode for {_btDevID}')
                        
                    return True
        
            except IOError as e:
                logger.error(f'BT Config update error({0}): {1} {buf}'.format(e.errno, e.strerror))
                return False
            except: 
                logger.error(f'Unexpected error - {sys.exc_info()[0]} {buf}')
                return False

    else:
        logger.error(f'Updating bluetooth config, file does not exist {config.args.bt_config_path}')
        return False
