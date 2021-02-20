# -*- coding: utf-8 -*-
# ---------------------------------------------------------------------------------
# 
#  LMS Server routines:  
#       get_lms_request()
#       get_lms_callback()  
#       get_player_status()
#       lms_monitor()
#       lms_initialise()
#       send_lms_command()
#       get_player_mode()
#
# ---------------------------------------------------------------------------------
import  config
import  asyncio
from    random import seed, randint
from    urllib.parse import unquote

from network_check   import network_check
from network_check   import host_connect

import  logging
logger = logging.getLogger(__name__)

class lms_Controller:

    def __init__(self):

        logger.info('Initialising LMS Controller')
        self._reader = None
        self._writer = None
        self._playerID = None
        self._playerCount = 0
        self._playersSynced = ''
        self._playerMode = ''

        seed(2)

    # ---------------------------------------------------------------------------------
    #
    # get_lms_request()
    #       - Make a request to LMS server, wait for a response and return  
    #       - Command index can be found at:
    #           http://[LMS SERVER IP]:9000/html/docs/cli-api.html?player=
    #
    # ---------------------------------------------------------------------------------
    async def get_lms_request(self, _message):
        
        logger.debug(f'Send: {_message!r}')

        try:
        
            self._writer.write(f'{_message}\n'.encode("utf-8"))
            await self._writer.drain()
        
        except Exception as e:
            logging.error(f'asyncio.write failed {_message} - {e}')
            raise Exception('get_lms_request(): Network Error')

        try:
        
            data = await self._reader.readline()
            response = unquote(str(data.decode("utf-8").strip()))
            logger.debug(f'Recv: {response!r}')
            
            # LMS returns the request + the reponse, return just the response portion
            c = ''
            for c in range(len(response)):
                if response[c] != _message[c]:
                    return response[c:len(response)] 

        except Exception as e:
            logging.error(f'asyncio.read failed - {e}')
            raise Exception('get_lms_request(): Network Error')
        

    # ---------------------------------------------------------------------------------
    #
    # get_player_status()
    #       - Get currentl status of the in specified player
    #           a) playing mode (play, pause, stop)
    #           b) player ID (MAC address)
    #           c) Sync group
    #
    # ---------------------------------------------------------------------------------
    async def get_player_status(self):

        try:
            if self._playerID is None:   
                await self.get_player_ID()
        
            # Get player status
            if self._playerID:
                self._playerMode = await self.get_lms_request( '{} mode ?'.format(self._playerID))
                self._playersSynced = await self.get_lms_request( '{} sync ?'.format(self._playerID))
                self._playersSynced = self._playersSynced.split(',')
                logger.debug(f'player status {self._playerID} {config.args.player_name} {self._playerMode} {self._playersSynced}')

            else:
                logger.warning(f'player NAME not found, check squeezelite is running and player name in config file is correct - {config.args.player_name}')
        
        except Exception as e:
            logging.warning(f'player status exception - {e}')  # Ignore errors here

    # ---------------------------------------------------------------------------------
    #
    # get_player_ID()
    #       - Return the player ID (MAC Address) or if not found return none
    #
    # ---------------------------------------------------------------------------------
    async def get_player_ID(self):
    
        try:
            # Get a list of players.  Assume a max of 64
            self._playerCount = await self.get_lms_request('player count ?')

            #  cycle through the player list to find it
            if int(self._playerCount) > 0 and int(self._playerCount) < 64:  
                i = 0          
                for i in range(int(self._playerCount)):
                    name = await self.get_lms_request( "player name {} ?".format(i))

                    if name.lower() == config.args.player_name.lower():
                        self._playerID = await self.get_lms_request( "player id {} ?".format(i))
                        logger.info(f'Found registered player ({self._playerID}, {config.args.player_name})')
                        break   # Found, no need to contine looping

            else:
                raise Exception('lms_initialise(): Too many register players max 64') 
        
        except Exception as e:
            logger.warning(f'get_player_ID failed - {e}')


    # ---------------------------------------------------------------------------------         
    #
    # get_lms_callback()
    #       - Wait for notification from a LMS subscribe request
    #       - http://[LMS SERVER IP]:9000/html/docs/cli-api.html?player=#subscribe
    #       - If a network error invalidate read/write pair so LMS retry occurs
    #
    # ---------------------------------------------------------------------------------
    async def get_lms_callback(self):

        try:

            data = await self._reader.readline()

        except Exception as e:
            logging.error(f'LMS readline() failed - {e}')
            self._writer = None
            self._reader = None
            return ''

        response = unquote(str(data.decode("utf-8").strip()))
        logger.debug(f'LMS response: {response!r}')
        
        return response.split(' ')

    # ---------------------------------------------------------------------------------
    #
    #  Called form main()
    #           - Loop to monitor notifications from LMS server and set LED's
    #           - Factors in if player is in a sync group
    #           - If there is a network failure will try to reconnect to server
    #
    # ---------------------------------------------------------------------------------
    async def lms_monitor(self, gpio):

        while True:

            if self._writer:                                # Check we are still connected

                try:      
                                    
                    event = await self.get_lms_callback()   # Wait for notification from LMS

       #     Only works in 3.7+
       #             try:
       #                 event = await asyncio.wait_for(
       #                     asyncio.gather(self.get_lms_callback()),
       #                     timeout=5.0
       #                 )
       #                 
       #             except asyncio.TimeoutError:
       #                 logger.debug("Callback timeout")
       #   
       #             else:    
        
                    await self.get_player_status()          # Update players, playing mode, sync groups

                    for player in self._playersSynced:      # Check to see if the notification is from our chosen 
                                                            # player or its sync group
                        
                        # Notification event format <player ID> playlist newsong|play|stop|pause
                        if event[0] == self._playerID or event[0] == player:
                            logger.debug(f'message ({event})')
                            if event[1] == 'playlist':
                                if event[2] == 'newsong':
                                    self._playerMode  = 'play'
                                elif event[2] == 'stop':
                                    self._playerMode  = 'stop'
                                elif event[2] == 'pause' and event[3] == 0:
                                    self._playerMode  = 'play'
                                elif event[2] == 'pause' and event[3] == 1:
                                    self._playerMode  = 'pause'
        
                except Exception as e:
                    logger.warning(f'LMS monitor - {e}')
     
            else:
                # Retry connection to LMS Server and re-init player config
                try:
                    await lms_Controller.lms_initialise(self, gpio)
                except Exception as e:
                    self.playerMode = 'error'
                    logger.error(f'LMS retry {e}')

  
    # ---------------------------------------------------------------------------------
    #
    # lms_initialise():
    #           - Connect to LMS Server
    #           - Get client player identifier (MAC Address)
    #           - Start up pause mode
    # 
    # ---------------------------------------------------------------------------------
    async def lms_initialise(self, gpio):

        # Connect to LMS host
        self._reader, self._writer = await host_connect(
                    gpio, config.args.lms_ip, config.args.lms_port, 
                    _retry_delay_min=2, _retry_delay_max=10, _led=config.args.led3)

        if self._writer:   # If connected to LMS server

            logger.info(f'LMS connection SUCCESSFUL')

            await self.get_player_status()    
    
            if self._playerID is not None:
                lms_Controller.send_lms_command(self, 'stop')


            #  - Only recieve 'playlist,sync,client' notifications to reduce activity
            #    <player id > playlist pause 0 == play
            #    <player id > playlist pause 1 == pause
            #    <player id > playlist newsong 0 == play
            #   Sync and client so we get any changes in players
            try:
        
                await self.get_lms_request("subscribe playlist,client,sync")

            except Exception as e:
                raise Exception(f'lms_initialise(): LMS subscribe failed - {e}')  

            logger.debug(f'initalised OK')

        else:
            raise Exception('lms_initialise(): Not connected to LMS')

    # ---------------------------------------------------------------------------------
    #
    #  send_lms_command()
    #       - Used by gpio_controller to send commands
    #       - Does not wait for a response as get_lms_callback is probably waiting
    #       - Invalidate read/write pair so connection is retried
    #
    # ---------------------------------------------------------------------------------
    def send_lms_command(self, _command):
        logger.info(f'{self._playerID} {_command}')
        
        try:
        
            self._writer.write(f'{self._playerID} {_command}\n'.encode("utf-8"))
        
        except Exception as e:
            logger.error(f'asyncio write failed - {e}')
            self._writer = None
            self._reader = None
    
    # ---------------------------------------------------------------------------------
    #
    # get_player_mode()
    #       - Used by gpio_controller to get playing mode - Used to set the LEDs
    #
    # ---------------------------------------------------------------------------------
    def get_player_mode(self):
        
        return self._playerMode
