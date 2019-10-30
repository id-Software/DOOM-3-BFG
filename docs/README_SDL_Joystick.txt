This file documents the use of a joystick/gamepad under Linux with RBDoom3-BFG.
Joystick support has been tested with an XBox 360 wireless and a
Logitech F710 wireless gamepads, but in theory should work with any XBox 360
compatible gamepad, either wireless or wired/USB.

To use the gamepad follow this instructions,

a) Make sure your joystick is detected by Linux.

One way to check this is to look for jsN files under /dev/input/, where
N is a number. For example, /dev/input/js0 is the first joystick, 
/dev/input/js1 the second one, and so on. Executing the following command you
should see something like this:

 $ ls -l /dev/input/js*
 crw-rw-r--+ 1 root input 13, 0 Nov 20 04:39 /dev/input/js0

Another way to check is to use the udevadm monitor command. This is what happens
when you connect a USB joystick (launch udevadm and then connect the gamepad or
the wireless receiver):

 $ udevadm monitor -u
 monitor will print the received events for:
 UDEV - the event which udev sends out after rule processing

 UDEV  [14503.227311] add /devices/pci0000:00/0000:00:1d.0/usb5/5-2 (usb)
 UDEV  [14504.551280] add /devices/pci0000:00/0000:00:1d.0/usb5/5-2/5-2:1.0 (usb)
 UDEV  [14504.553328] add /devices/pci0000:00/0000:00:1d.0/usb5/5-2/5-2:1.0/input/input22 (input)
 UDEV  [14504.555805] add /devices/pci0000:00/0000:00:1d.0/usb5/5-2/5-2:1.0/input/input22/event19 (input)
 UDEV  [14504.565611] add /devices/pci0000:00/0000:00:1d.0/usb5/5-2/5-2:1.0/input/input22/js0 (input)

The program jstest allows you to check if your joystick works OK:

 $ jstest /dev/input/js0
 Driver version is 2.1.0.
 Joystick (Xbox 360 Wireless Receiver) has 6 axes (X, Y, Z, Rx, Ry, Rz)
 and 15 buttons (BtnX, BtnY, BtnTL, BtnTR, BtnTR2, BtnSelect, BtnThumbL, BtnThumbR, ....
 Testing ... (interrupt to exit)

Every time you press a button or move an axis you should see the changes printed on
the screen. Press CTRL+C to end jstest.

No matter how many joysticks you have, RBDoom3-BFG only uses the first 
joystick named /dev/input/js0.

b) Make sure RBDoom3-BFG detects and is able to use your joystick.

Execute RBDoom3-BFG in a console/terminal to see the messages printed there. If
your joystick is correctly detected you should see something like this:

 $ ./RBDoom3BFG

 ... lots of text ...

 ----- R_InitOpenGL -----
 Initializing OpenGL subsystem
 Using 8 color bits, 24 depth, 8 stencil display
 Using GLEW 1.10.0
 Sys_InitInput: Joystick subsystem init
 Sys_InitInput: Joystic - Found 4 joysticks
 Opened Joystick 0
 Name: Xbox 360 Wireless Receiver
 Number of Axes: 6
 Number of Buttons: 15
 Number of Hats: 0
 Number of Balls: 0

 ... more text ...

If the number of detected joysticks is 0 and none is opened check again
a) and make sure the file /dev/input/js0 is there (for example, if you
are using a wireless joystick make sure the gamepad is ON and not
in sleeping mode when you run RBDoom3-BFG) and that you have permissions
to read that file (for example, in Debian users that want to use the 
joystick must be added to the group input).

c) Set the bindings of your joystick.

You can use the Doom3 console to set the bindings. To bind a joystick
button/axis to a game action, press the tilde key (just below ESC) to 
show the Doom3 console. Then type:

] bind <key_name> <action>

For example, this binds gamepad button 0 with the action "jump":

] bind JOY1 _moveup

Apendix A gives you a list of the gamepad axis/button names and the
possible actions you can bind to them.

Alternatively, you can use configuration files to set the bindings
automatically. Copy the files joy_righty.cfg, joy_lefty.cfg and joy_360_0.cfg
to your Doom3 base directory. You can find these files in Appendix B at the
bottom of this file.

d) Enjoy!

--------------------------------------------------------------------------
TODO/BUGS

 * Only XBox 360 gamepad clones are supported at the moment.

 * SDL axis/keycode numbers are hard-coded to Doom3 axis/keycode names.
Depending of your gamepad model maybe this will produce unwanted results.
It is not clear at the moment how to remap this (configuration file, GUI, ...).

--------------------------------------------------------------------------
Apendix A) Doom3 joystick key names and key bindings

This is a list of the supported key/axis names. Note that the names of the
buttons are only valid for the XBox 360 wireless gamepad. Other gamepads
may have different names depending on the gamepad layout and/or more buttons
available so you can map more actions. Also note that RBDoom3-BFG treats 
the back triggers like buttons rather that axes.

JOY1  (A button)
JOY2  (B/Circle button)
JOY3  (X/Square button)
JOY4  (Y/Triangle button)
JOY5  (L Shoulder)
JOY6  (R Shoulder)
JOY7  (BACK)
JOY8  (START)
JOY9  (XBOX button)
JOY10 (LEFT ANALOG JOY BUTTON)
JOY11 (RIGHT ANALOG JOY BUTTON)

JOY_TRIGGER1 (LEFT TRIGGER)
JOY_TRIGGER2 (RIGHT TRIGGER)

JOY_DPAD_RIGHT   (D-PAD ANALOG JOYSTIC UP)
JOY_DPAD_LEFT    (D-PAD ANALOG JOYSTIC DOWN)
JOY_DPAD_UP      (D-PAD ANALOG JOYSTIC LEFT)
JOY_DPAD_DOWN    (D-PAD ANALOG JOYSTIC RIGHT)

JOY_STICK1_UP    (RIGHT ANALOG JOYSTIC UP)
JOY_STICK1_DOWN  (RIGHT ANALOG JOYSTIC DOWN)
JOY_STICK1_LEFT  (RIGHT ANALOG JOYSTIC LEFT)
JOY_STICK1_RIGHT (RIGHT ANALOG JOYSTIC RIGHT)
JOY_STICK2_UP    (LEFT ANALOG JOYSTIC UP)
JOY_STICK2_DOWN  (LEFT ANALOG JOYSTIC DOWN)
JOY_STICK2_LEFT  (LEFT ANALOG JOYSTIC LEFT)
JOY_STICK2_RIGHT (LEFT ANALOG JOYSTIC RIGHT)

This is a list of commands that can be binded in Doom3. This commands are
only valid in-game, the GUI uses its own set of hard-coded key bindings. Note
that commands having spaces in between (more than one word) must be quoted "":

_impulse0   Fists / Grabber
_impulse2   Pistol
_impulse3   Shotgun / Double shotgun (Doomclassic)
_impulse5   Machinegun
_impulse6   Chaingun
_impulse7   Grenades
_impulse8   Plasma Gun
_impulse9   Rockets
_impulse10  BFG 9000
_impulse12  Soulcube / Artifact
_impulse13  Reload weapon
_impulse14  Previous weapon
_impulse15  Next weapon
_impulse16  Flashlight
_impulse19  PDA / Scoreboard

_attack     Fire / Melee
_use        Use / Action
_speed      Run

_forward    Move forward
_back       Move backwards
_moveleft   Move left
_moveright  Move right
_moveup     Jump
_movedown   Crouch
_left       Look left
_right      Look right
_lookup     Look up
_lookdown   Look down

"savegame quick"  Quick Save
"loadgame quick"  Quick Load
"screenshot"      Take screenshot

Apendix B) Joystick bindings

-- joy_righty.cfg --------------------------------------------------------
bind "JOY_STICK1_UP"    "_forward"
bind "JOY_STICK1_DOWN"  "_back"
bind "JOY_STICK1_LEFT"  "_moveleft"
bind "JOY_STICK1_RIGHT" "_moveright"

bind "JOY_STICK2_UP"    "_lookup"
bind "JOY_STICK2_DOWN"  "_lookdown"
bind "JOY_STICK2_LEFT"  "_left"
bind "JOY_STICK2_RIGHT" "_right"
--------------------------------------------------------------------------

-- joy_lefty.cfg ---------------------------------------------------------
bind "JOY_STICK2_UP"    "_forward"
bind "JOY_STICK2_DOWN"  "_back"
bind "JOY_STICK2_LEFT"  "_moveleft"
bind "JOY_STICK2_RIGHT" "_moveright"

bind "JOY_STICK1_UP"    "_lookup"
bind "JOY_STICK1_DOWN"  "_lookdown"
bind "JOY_STICK1_LEFT"  "_left"
bind "JOY_STICK1_RIGHT" "_right"
--------------------------------------------------------------------------

-- joy_360_0.cfg ---------------------------------------------------------
bind "JOY_DPAD_RIGHT" "_impulse5"  // Put your favourite weapons here
bind "JOY_DPAD_LEFT"  "_impulse12" // for quick access.
bind "JOY_DPAD_UP"    "_impulse6"
bind "JOY_DPAD_DOWN"  "_impulse8"

bind "JOY_TRIGGER1"   "_impulse16"
bind "JOY_TRIGGER2"   "_attack"

bind "JOY1" "_moveup"     // (A/X button)
bind "JOY2" ""            // (B/Circle button)
bind "JOY3" "_impulse13"  // (X/Square button)
bind "JOY4" "_use"        // (Y/Triangle button)

bind "JOY5" "_impulse15"  // (L Shoulder)
bind "JOY6" "_impulse14"  // (R Shoulder)

bind "JOY7" "_impulse19"  // (BACK)
bind "JOY8" ""            // (START) (Used to show the game GUI)
bind "JOY9" ""            // (XBOX)

bind "JOY10" "_speed"     // (LEFT ANALOG JOY BUTTON)
bind "JOY11" "_movedown"  // (RIGHT ANALOG JOY BUTTON)
--------------------------------------------------------------------------
