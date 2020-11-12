# AudioPot

AudioPot is the software implementation for my audio volume knob project. Basically, I connected a potentiometer to an Arduino, and I output the values I read off it with the ADC to the serial port. On the client (works on Windows, macOS), I set the system volume proportional to the received value. Also, the potentiometer can be pressed down, triggering the reset button on the Arduino, which plays/pauses current media on the computer.

Binaries (for Windows only) are available in [Releases](https://github.com/valinet/AudioPot/releases).

#### Concepts shown in this example:

* Arduino: Basic example of analogRead
* Windows: Receiving device tree change notifications via the window message queue (I tried to use WMI, succeeded, but some sort of bug made the CPU usage of WMI Provider Host rise to 100%; also, why bother with all that overhead when doing it via the message queue is so elegant)
* Windows: Reading data form the serial port asynchronously (using overlapped I/O) - not really necessary for the current state of the project, but I was planning something bigger
* macOS (UNIX): Configuration and synchronous read from serial port
* Windows: Persisting the daemon by implementing a Windows service for it (the service spawns a second process on the *WinSta0* station so that the application is able to change volume)
* Windows & macOS: Changing system audio volume -  sounds trivial, but you'd never guess...

## Windows

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

To compile, open the provided solution file in Visual Studio or use MSBuild in a command window:

```
msbuild AudioPot.sln /property:Configuration=Release /property:Platform=x64
```

## macOS

I provide a simple Objective-C source file that opens the serial port where the device is attached, continuously reads data and changes the system volume. To compile this file, run:

```
clang -o wheel wheel.m -framework IOKit -framework Cocoa
```

For this to work, you need to install Xcode. Then, all you need to do is run *wheel*. At the moment of writing this, it does not support hot plugging the device (i.e. the daemon has to be restarted if you unplug and replug the device).

To finish off, you can use Automator, for example, to launch this at startup as described [here](https://superuser.com/questions/229773/run-command-on-startup-login-mac-os-x).

## GNU/Linux

This can easily work on GNU/Linux as well. The macOS version can be used as base. One needs to change the serial device name. Also, the macOS specific code which changes the volume has to be replaced with something that presses the volume keys programatically. Two implementations are probably needed, one for X11, and one for Wayland. For X, I remember from some time ago that the keyword is *xF86XK_AudioRaiseVolume* etc. Sending keys is the easiest, alternatively one may try talking with the window manager over its exposed D-Bus interfaces, but that means particular implementations have to be done for GNOME, KDE etc. Pressing the volume keys will trigger the respective handler in any DE, should it have one. If you are in a lightweight DE that does not handle the volume keys in any way, you can use mixer to set the volume.