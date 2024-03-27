The original Settings.ini approach required repeated reading of the file and
also required a line in the file for each temperature setting change, even
when following a linear ramp.  This meant a compromise between choppy inputs
and excessive data entry.

As before, the file name must be Settings.ini and it must be in the root
directory of the SD card.

The new format and sketch overcome this and add several features.
1) Interpolation.  By default, target temperatures are linearly interpolated
   between the specified points.  This is more convenient and more importantly
   less error-prone.  It is best practice to have a new line in the file only
   when at least one tank has a change of slope in its temperature plan.

   Interpolation options are:
INTERP LINEAR   // the default
INTERP STEP	// Change only at explicitly-entered points.  Probably only for mimicing old results.
   Other options such as a cubic spline could be interesting, but are not implemented yet.

2) Start time.  Start times are not always predictable when working with live
   specimens gathered from the sea. Rather than changing the time for each line
   in the input file, the standard is now to start the ramp definition at time
   0:00 and use START to specify the time of day to start.

   For example:
START 13:30   // Start at 1:30 PM CBASS time

3) The actual ramp definition looks like this.

0:00	26	26	26	26
3:00	28	30	32	34
6:00	28	30	32	34
7:00	26	26	26	26

With this definition temperatures start to increase at START, and continue for
3 hours.  They then hold for three hours before dropping to the original level
over a period of 1 hour.  Since the start and end temperatures are the same,
holding at that level for the rest of the 24-hour cycle is implied.

More complex ramp plans are easily supported by entering more lines.  The
default limit of 20 lines can easily be increased in Settings.h
(MAX_RAMP_STEPS).

A fine point on accuracy: ramp point temperatures are stored to the nearest
0.01 degree C, but interpolated values use full floating-point precision.
Note that even 0.01 is overkill since the claimed accuracy of the CBASS
sensors (and the HOBO loggers often used as backup) is only 0.5 C. 

Someone should really evaluate whan accuracy can be obtained with a little
calibration...
