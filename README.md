# CBASS-32
CBASS refers to any version of the [Coral Bleaching Automated Stress System](https://aslopubs.onlinelibrary.wiley.com/doi/pdfdirect/10.1002/lom3.10555), as developed by several researchers.  CBASS-32 is a specific implementation of the controller using an Arduino Nano ESP-32 to support additional features.

This repository contains main and supporting sketches for CBASS-32.  

## CBASS-32
This is the main sketch to run experiments.  Extensive documentation is in the [CBASS-32 User Guide](https://tinyurl.com/CBASS-32) google doc,
which is still being completed.
You will be able to control up to 8 experimental tanks from a single compact CBASS-32 system.
Thermal control and primary data logging is the same as on legacy systems.  The new system has a web interface.  This supports
the following actions without physical access to the system's microSD card:
1. Edit the ramp plan.
2. Change the start time of the ramp.
3. Synchronize the real time clock to the attached device.
4. Download the LOG.txt file.
5. Roll over the LOG.txt file into a backup directory and start a new one.
6. Monitor temperatures on a live graph.
7. List the files on the microSD card.
8. Reset the ramp plan to a default example.
9. Reboot CBASS-32

## SPIFFS_upload
The web interface depends on several CSS, Javascript, and icon files.  This tool allows
them to be moved from an installed microSD card to the ESP32's internal filesystem.  This
has proven faster and more reliable than the SD card, so it is mandatory.  Brief instructions
are in the *.ino file.

## References
The main methods paper for CBASS is:

Evensen, N. R., Parker, K. E., Oliver, T. A., Palumbi, S. R., Logan, C. A., Ryan, J. S., ... & Barshis, D. J. (2023). [The coral bleaching automated stress system (CBASS): a low‐cost, portable system for standardized empirical assessments of coral thermal limits.](https://aslopubs.onlinelibrary.wiley.com/doi/pdfdirect/10.1002/lom3.10555) Limnology and Oceanography: Methods, 21(7), 421-434.
