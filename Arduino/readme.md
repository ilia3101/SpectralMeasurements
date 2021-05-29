### Compiling

1. Get arduino-cli and avr package
```
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=./bin sudo sh
./bin/arduino-cli core install arduino:avr
```

2. Compile
From above directory (not from in here):
```
./bin/arduino-cli compile -b arduino:avr:nano Arduino
./bin/arduino-cli upload -p /dev/ttyUSB0 -b arduino:avr:nano:cpu=atmega328old Arduino
```
May require a different ttyUSB number