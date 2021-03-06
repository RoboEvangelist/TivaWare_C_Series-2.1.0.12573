# File: server.py

'''
Client specifically for controlling several rovers and gather data
from the rovers
'''

import kivy
kivy.require('1.8.0')
import StringIO
from kivy.uix.button import Button
from kivy.clock import Clock
from kivy.logger import Logger
from kivy.uix.slider import Slider
from kivy.uix.floatlayout import FloatLayout
from kivy.app import App
from kivy.graphics import Rectangle
from kivy.core.image.img_pygame import ImageLoaderPygame
from kivy.uix.widget import Widget
from kivy.uix.label import Label

import socket

#import roslib; roslib.load_manifest('br_swarm_rover')
#import rospy 
#from std_msgs.msg import String
#from sensor_msgs.msg import CompressedImage

# FIXME this shouldn't be necessary
from kivy.base import EventLoop
EventLoop.ensure_window()

class KeyboardInterface():
    def __init__(self, server_uri, update_image):

        self.__has_control = True

    def hasControl(self):
        '''
        Return True if user wants to send control commands with
        keyboard.
        Tab key and shift-tab toggle this.
        '''
        # Check if control flag should be toggled.
#        #import #pygame
#        if pygame.key.get_pressed()[pygame.K_TAB]:
#            self.__has_control = not (pygame.key.get_mods() &
#                                    pygame.KMOD_LSHIFT)
#            if self.__has_control:
#                print('Take control')
#            else:
#                print('Release control')
#
#        return self.__has_control

class ControlClass(FloatLayout):
    def __init__(self, **kwargs):
        '''
        We don't use __init__ because some variables cannot be initialized
        to an ObjectProperty for some reason, so we gotta initialize
        it this way
        '''
        super(ControlClass, self).__init__(**kwargs)
        self._started = False         # true if client connected
	self._binded = False
        self._client = None           # server client object
        self._ros_uri = []      # stores robots ID plus rus URI
        self._robot_id = "1"      # default ID of robot to command
	# there are 5 velocities: -2, -1, 0, 1, 2
	self._velocity = "0" 
	# autonomous mode on is 1...off is 0
	self._autonomous = 0
        
        # normal image widget
        self.norm_im_widget = Widget()
        self.add_widget(self.norm_im_widget) 
        # original image size (from server)
        self._im_width = 320.0
        self._im_height = 240.0

        # variables for publishing movement
#        self._pub = rospy.Publisher('move', String)
#        rospy.init_node('br_gui')
#        self._im_string = ''      # image string
        self._im_string = []      # image string
        self._tcp_ip = '192.168.1.134'   # robot's IP address
        self._tcp_port = 5005   
        self._rec_ip = '192.168.1.110'   # computer's IP address
        self._rec_port = 5006   
	self._conn = ""
	self.addr = ""
        self._message_buffer_size = 255 
        
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._rec_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def get_image_data(self, image_buffer):
        '''
        obtains the published compressed image data
        '''
        self._im_string[int(self._robot_id)-1] = image_buffer.data

    def selected_robot(self, robot_menu, robot):
        '''
        selected a robot to drive throgh the drop down menu
        '''
        self._robot_id = robot
        Logger.info('Robot #' + robot + ' has been selected') 
#        print('Robot #', robot, ' has been selected')

    def start_autonomous(self, *args):
        '''
	Turns autonomous mode on/off
        '''
        try:
#            self._pub.publish(String('stop'+str(self._robot_id)))
            # rotate between on (1) or off (0)
            self._autonomous = 1 - self._autonomous
            self._socket.send("auto" + str(self._autonomous))
	    # continuously send command to avoid socket blocking the program
	    if self._autonomous:
                Clock.schedule_interval(self.send_auto_command, 0.1)
	    # else, don't send the autonomous command again
            else:
                Clock.unschedule(self.send_auto_command)
	    print "autonomous mode: " + str(self._autonomous)
        except BaseException, e:
            print "Stop Tracks Service call failed: %s"%e

    def send_auto_command(self, *args):
        '''
	Continuously send autonomous command
        '''
        try:
            # rotate between on (1) or off (0)
            self._socket.send("auto1")
            try:
    		data = self._socket.recv(self._message_buffer_size)
		print "data received: " + data
            except BaseException:
		print "could not receive data"
#		pass
        except BaseException, e:
            print " %s"%e

    def call_stop_track(self, *args):
        '''
        publishes the stop command to stop a robot if a move button
	is no longer pressed
        '''
        try:
#            self._pub.publish(String('stop'+str(self._robot_id)))
            self._socket.send("STOP")
            print "stop tracks"
        except BaseException, e:
            print "Stop Tracks Service call failed: %s"%e

    def call_move_forward(self, *args):
        '''
        calls the move forward service to move robot forward
        '''
        try:
#            self._pub.publish(String('forward'+str(self._robot_id)))
            self._socket.send("F" + self._velocity )
        except BaseException, e:
            print "Move forward Service call failed: %s"%e

    def call_move_backward(self, *args):
        '''
        calls the move backward service to move
        '''
        try:
#            self._pub.publish(String('backward'+str(self._robot_id)))
            self._socket.send("B" + self._velocity)
        except BaseException, e:
            print "Move backward Service call failed: %s"%e

    # TODO: fix the issue when (e.g. 'turn left' and 'left') commands
    # are interpreted as the same when python is analysing the
    # string being publish
    # initials are being used instead of complete words as a
    # work-around the problem

    def call_turn_left(self, *args):
        '''
        calls the turn left service to move robot
        '''
        try:
#            self._pub.publish(String('TuLef'+str(self._robot_id)))
            self._socket.send("L" + self._velocity)
        except BaseException, e:
            print "Move turn left Service call failed: %s"%e

    def call_turn_right(self, *args):
        '''
        calls the turn right service to move
        '''
        try:
#            self._pub.publish(String('TuRi'+str(self._robot_id)))
            self._socket.send("R" + self._velocity)
        except BaseException, e:
            print "Move turn right Service call failed: %s"%e

    def call_left_forward(self, *args):
        '''
        calls the turn left forward service to move
        '''
        try:
#            self._pub.publish(String('LefFor'+str(self._robot_id)))
            self._socket.send("LF" + self._velocity)
        except BaseException, e:
            print "Move left forward Service call failed: %s"%e

    def call_right_forward(self, *args):
        '''
        calls the turn right forward service to move
        '''
        try:
#            self._pub.publish(String('RiFor'+str(self._robot_id)))
            self._socket.send("RF" + self._velocity)
        except BaseException, e:
            print "Move right forward Service call failed: %s"%e

    def call_left_backward(self, *args):
        '''
        calls the turn left backward service to move
        '''
        try:
#            self._pub.publish(String('LefBa'+str(self._robot_id)))
            self._socket.send("LB" + self._velocity)
        except BaseException, e:
            print "Move left backward Service call failed: %s"%e

    def call_right_backward(self, *args):
        '''
        calls the turn right backward service to move
        '''
        try:
#            self._pub.publish(String('RiBa'+str(self._robot_id)))
            self._socket.send("RB" + self._velocity)
        except BaseException, e:
            print "Move right backward Service call failed: %s"%e

    def velocity_slider_value(self, instance, value): 
        # this will print a ref to the slider and its value
        # these values are passed to this func as arguments
        # through kv  `on_value: root.slider_value`
        print instance, value
	# eliminate 0 from the string
	self._velocity = str(int(value))

    def start_server(self, dt):
        '''
        initializes the client connection, and gets image data
        '''
        Logger.info('trying to connect to server... \n')
        if not self._started:
            count = 0
            while True:  # Keep trying in case server is not up yet
                try:
                    # connect to meta server first
                    self._socket.connect((self._tcp_ip, self._tcp_port))

                    Logger.info('Socket Connected! \n')
#                    Logger.info(data)
                    self._started = True
                    break
                except socket.error:
                    count += 1
                    import time
                    time.sleep(.5)

                if count > 100:
                    waiting = 'Waiting for server to connect'
                    Logger.info(waiting)

    def stop_connection(self, *args):
	Logger.info('trying to disconnect from server... \n')
        try:
            self._socket.close()
	    self._started = False
            Logger.info('Socket Disconnected! \n')
	except socket.error:
	    print "socket was not previously connected"
        

    # create threads down here

    def schedule_client(self, *args):
        '''
        starts the thead to run the server simulation
        '''
        # called only when button is pressed
        trigger = Clock.create_trigger(self.start_server)
        # later
        trigger()
        # schedule image display thread
        #Clock.schedule_interval(self.display_raw_image, 1.0 / 30.0)

    def unschedule_client(self, *args):
        '''
        starts the thead to run the server simulation
        '''
        # called only when button is pressed
        trigger = Clock.create_trigger(self.stop_connection)
        # later
        trigger()

    def schedule_automation(self, *args):
        '''
        starts the thead to run the server simulation
        '''
        # called only when button is pressed
        trigger = Clock.create_trigger(self.start_autonomous)
        # later
        trigger()

    def schedule_stop_track(self, *args):
        '''
        calls the stop track on a loop
        '''
        trigger = Clock.create_trigger(self.call_stop_track)
        # later
        trigger()

    def schedule_move_forward(self, *args):
        '''
        calls the move forward on a loop
        '''
        # this function is called only when button is pressed
        trigger = Clock.create_trigger(self.call_move_forward)
        # later
        trigger()

    def schedule_move_backward(self, *args):
        '''
        calls the move backward on a loop
        '''
        # this function only when button is pressed
        trigger = Clock.create_trigger(self.call_move_backward)
        # later
        trigger()

    def schedule_turn_left(self, *args):
        '''
        calls the turn left on a loop
        '''
        # this function is called only when button is pressed
        trigger = Clock.create_trigger(self.call_turn_left)
        # later
        trigger()

    def schedule_turn_right(self, *args):
        '''
        calls the turn left on a loop
        '''
        # this function is called only when button is pressed
        trigger = Clock.create_trigger(self.call_turn_right)
        # later
        trigger()

    def schedule_left_forward(self, *args):
        '''
        calls the left forward on a loop
        '''
        # this function is called only when button is pressed
        trigger = Clock.create_trigger(self.call_left_forward)
        # later
        trigger()

    def schedule_right_forward(self, *args):
        '''
        calls the right forward on a loop
        '''
        # this function is called only when button is pressed
        trigger = Clock.create_trigger(self.call_right_forward)
        # later
        trigger()

    def schedule_left_backward(self, *args):
        '''
        calls the left backward on a loop
        '''
        # this function is called only when button is pressed
        trigger = Clock.create_trigger(self.call_left_backward)
        # later
        trigger()

    def schedule_right_backward(self, *args):
        '''
        calls the right backward on a loop
        '''
        # this function is called only when button is pressed
        trigger = Clock.create_trigger(self.call_right_backward)
        # later
        trigger()

class KivyGui(App):
    try:
        def build(self):
            return ControlClass()
    except BaseException as error:
        print "error: ", error
        from sys import exit
        exit()

if __name__ == '__main__':
    KivyGui().run()
