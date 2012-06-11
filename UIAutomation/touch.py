# This is a monkeyrunner jython script that opens a connection to an Android
# device and continually sends a stream of swipe and touch gestures.
#
# See http://developer.android.com/guide/developing/tools/monkeyrunner_concepts.html
#
# usage: monkeyrunner swipe_monkey.py
#

# Imports the monkeyrunner modules used by this program
from com.android.monkeyrunner import MonkeyRunner, MonkeyDevice

# Connects to the current device
device = MonkeyRunner.waitForConnection()

# A swipe left from (x1, y) to (x2, y) in 2 steps
y = 350
x1 = 50
#x2 = 300
#start = (x1, y)
#end = (x2, y)
#duration = 0.2
#steps = 2
#pause = 0.2
print "x,y" ,x1,y
device.touch(x1, y, 'DOWN_AND_UP')
##
##for i in range(1, 250):
##    # Every so often inject a touch to spice things up!
##    if i % 9 == 0:
##        device.touch(x1, y, 'DOWN_AND_UP')
##        MonkeyRunner.sleep(pause)
##    # Swipe right
##    device.drag(start, end, duration, steps)
##    MonkeyRunner.sleep(pause)
##    # Swipe left
##    device.drag(end, start, duration, steps)
##    MonkeyRunner.sleep(pause)
##
