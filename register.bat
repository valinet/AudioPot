sc create AudioPot binPath= "C:\Users\Valentin\Documents\Visual Studio 2019\Projects\AudioPot\x64\Release\AudioPot.exe" DisplayName= AudioPot start= auto
sc description AudioPot "AudioPot service (https://github.com/valinet/AudioPot)"
sc start AudioPot