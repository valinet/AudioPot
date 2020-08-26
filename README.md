# AudioPot

AudioPot is the software implementation for my audio volume knob project. Basically, I connected a potentiometer to an Arduino, and I output the values I read off it with the ADC to the serial port. On the client (works on Windows), I set the system volume proportional to the received value. Also, the potentiometer can be pressed down, triggering the reset button on the Arduino, which plays/pauses current media on the computer

#### Concepts shown in this example:

* Basic Arduino example of analogRead
* Receiving device tree change notifications via the window message queue in Windows (I tried to use WMI, succeeded, but some sort of bug made the CPU usage of WMI Provider Host rise to 100%; also, why bother with all that overhead when doing it via the message queue is so elegant)
* Reading data form the serial port asynchronously (using overlapped I/O) - not really necessary for the current state of the project, but I was planning something bigger
* Persisting the daemon by implementing a Windows service for it (the service spawns a second process on the *WinSta0* station so that the application is able to change volume)
* Changing volume in Windows -  sounds trivial, but you'd never guess...

## Structure

The project is divided in:

* AudioPotArduino - Arduino code of the project, I tested it on an Arudino Nano
* AudioPotService - a service that starts up the daemon as soon as possible and restarts it in case of failure
* AudioPot - main app, communicates with Arduino and changes volume accordingly, if needed