# AudioPot

AudioPot is the software implementation for my audio volume knob project. Basically, I connected a potentiometer to an Arduino, and I output the values I read off it with the ADC to the serial port. On the client (works on Windows), I set the system volume proportional to the received value. Also, the potentiometer can be pressed down, triggering the reset button on the Arduino, which plays/pauses current media on the computer.

Binaries are available in [Releases](https://github.com/valinet/AudioPot/releases).

#### Concepts shown in this example:

* Basic Arduino example of analogRead
* Receiving device tree change notifications via the window message queue in Windows (I tried to use WMI, succeeded, but some sort of bug made the CPU usage of WMI Provider Host rise to 100%; also, why bother with all that overhead when doing it via the message queue is so elegant)
* Reading data form the serial port asynchronously (using overlapped I/O) - not really necessary for the current state of the project, but I was planning something bigger
* Persisting the daemon by implementing a Windows service for it (the service spawns a second process on the *WinSta0* station so that the application is able to change volume)
* Changing volume in Windows -  sounds trivial, but you'd never guess...

## Structure

The application is meant to be launched as a service. The service monitors the connection with the Arduino, and spawns a process of its own on the default window station, recreating in case it is killed or if it crashes. When launched as an executable, the application monitors for hardware changes and changes the volume based on commands received from the service though a names pipe. This is necessary because the service runs in a non interactive session, and thus cannot change desktop properties.

In order to register the application as a service, run this in an elevated command window:

```
sc create AudioPot binPath= "C:\...\AudioPot.exe" DisplayName= AudioPot start= auto
sc description AudioPot "AudioPot service (https://github.com/valinet/AudioPot)"
```

The service is set to start automatically at boot. To run immediatly, type:

```
sc start AudioPot
```

Also, in the AudioPot folder, you will find sketch.ino, which is the Arudino code I use.

* AudioPotArduino - Arduino code of the project, I tested it on an Arudino Nano
* AudioPotService - a service that starts up the daemon as soon as possible and restarts it in case of failure - register by running`sc create AudioPot binPath= C:\...\AudioPotService.exe` in an administrative command window
* AudioPot - main app, communicates with Arduino and changes volume accordingly, if needed