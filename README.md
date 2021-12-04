# Spectral Measurements

My camera spectral measurement repo with data and source code.

## Measured cameras

Cameras measured so far:

- Canon 5D Mark III (x2)
- Sony A7
- Lumix GH1
- Lumix GF3
- Pentax * IST DL

I have measured a few more cameras with a bad/old version of this setup, might upload that data to an 'old' folder.

I'm always looking for more cameras. Anyone in the UK willing to send me theirs for measurement?

## How it works

1. Full spectrum light passes through a motorised monochromator, which narrows the spectrum down to one wavelength at a time
2. For violet and red wavelengths the light is additionally filtered before entering the monochromator
3. The monochromatic light of unknown intensity then enters an integrating sphere, where it is measured by:
    1. photodiode with known response
    2. camera with unknown response
4. Readings from the photodiode and camera files are processed to calculate the camera's spectral response

The monochromator has a significant zero error, so it is always positioned at a known wavelength (632.8nm) using a Helium Neon laser *before taking any measurements.*

## The setup

In action!!!11!!1 (not really, the 12V and 5V cables aren't even plugged in) 

![In action](https://user-images.githubusercontent.com/23642861/144692391-57ce9639-8922-4689-961b-6ae15a40ab84.jpg)

Another angle:

![wires, foil and tape](https://user-images.githubusercontent.com/23642861/144686777-f576c793-c779-4354-8aaf-614a1862b31c.jpg)

## History

This spectral measurement rig started as a lockdown project (March 2020 era). Slowly been finding and upgrading components using ebay, I'm almost satisfied with the setup now.

## Upgrade ideas

Not cheap 😕

- Replace the TSL235 with a NIST traceable calibrated photodiode from Thorlabs
- Upgrade the lens from industar 61L/Z to something more neutral, maybe a Zeiss T*

Once these upgrades are done, it will be easy to correct the pre-upgrade data to match the post-upgrade data.
