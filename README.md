# Spectral Measurements

My camera spectral measurement repo with data and source code. Have been working on this since early 2020 on and off (mostly off).

<img src="https://user-images.githubusercontent.com/23642861/144692391-57ce9639-8922-4689-961b-6ae15a40ab84.jpg" height="370"> = <img src="https://user-images.githubusercontent.com/23642861/146609394-be3a07db-1217-4a57-93ca-8adcd098d4d2.png" height="370">

## Measured cameras

Cameras measured so far:

- Canon 5D Mark III (x2)
- Sony A7
- Sony A7 II
- Lumix GH1
- Lumix GF3
- Pentax * IST DL

I am remeasuring the listed cameras with the correctly working setup, and will upload data once they are done.

I have measured a few more cameras with a bad/old version of this setup, but I don't have access to those cameras anymore, so I won't be able to remeasure them.

I'm always looking for more cameras. Anyone want to send theirs for measurement?

## How it works

1. Full spectrum light passes through a motorised monochromator, which narrows the spectrum down to one wavelength at a time
2. For violet and red wavelengths the light is additionally filtered before entering the monochromator
3. The monochromatic light of unknown intensity then enters an integrating sphere, where it is measured by:
    1. photodiode with known response
    2. camera with unknown response
4. Readings from the photodiode and camera files are processed to calculate the camera's spectral response

The monochromator has a significant zero error, so it is always positioned at a known wavelength (632.8nm) using a Helium Neon laser before taking measurements (not every time, I have marked the position).

## Upgrade ideas

Not cheap 😕

- Replace the TSL235 with a NIST traceable calibrated photodiode from Thorlabs (or maybe something cheaper, but still better, I have a photodiode that I need to build a circuit for)
- Upgrade the lens from industar 61L/Z to something more neutral, maybe a Zeiss T*, or even use manufacturer-specific lenses to better match actual usage? Does this even matter? Not really.

Once these upgrades are done, it will be easy to correct the pre-upgrade data to match the post-upgrade data.


## Useful commands

Plot a graph
```
DAT='/path/to/response.dat'; gnuplot -e "unset ytics; plot '$DAT' using 1:2 with line lw 2 lc \"red\" notitle, '$DAT' using 1:3 with line lw 2 lc \"green\" notitle, '$DAT' using 1:4 with line lw 2 lc \"blue\" notitle; pause 1000;"
```

Add all camera data to git
```
git add Data/\*.txt Data/\*.dat
```
