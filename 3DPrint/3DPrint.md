# 3D Printer Files

These STL files may be used to print custom boxes for CBASS-32.  It is up to you
to chose your material and the best settings for your printer.

There is [more detail in the manual](https://tinyurl.com/CBASS-32).  Please read the 'Print a box' section there.

The files may change without warning, so if you have a favorite, keep a copy.  On
the other hand, new CBASS-32 versions may have some changes in dimensions, so always
check for new files when there is a new board version.

Currently we have four box versions
- RiblessWithNameplate_1DB9.stl
- RiblessWithNameplate_2DB9.stl
- StraightUSBC_1DB9.stl
- StraightUSBC_2DB9.stl

and two overlap files
- CuraOverlapSolids_OneDB9.stl
- CuraOvewrlapSolids_TwoDB9.stl

and one lid
- LidFor50x65plastic_9mmH.stl

The box versions starting with "Ribless" are for use with a Sparkfun USB-C panel mount
cable.  These are a tight fit in the box and require an extra part to "u-turn" the connection
at the board.  This led to the "StraightUSBC" versions which work with a straight USB-C
extender such as the [Cellularize USB C Extender](https://www.amazon.com/gp/product/B07MBWH7QG) .

If you will be controlling up to 4 tanks and no extra devices, the single DB9 versions are
recommended.  This saves one DB9 connector and means one less possible place for water intrusion.
With more than 4 tanks or if you will control tanks and lights, please use the double DB9 versions.

The lid file assumes that a plastic window of 50 x 65 mm will be glued in.  I use [Tap plastics
abrasion-resistent (AR) polycarbonate](https://www.tapplastics.com/product/plastics/cut_to_size_plastic/polycarbonate_sheets_ar/517) in 1/8" thickness.  Thinner plastic would be fine, and it
is probably cheaper to cut your own parts from a hardware store sheet if you only need a few.  In
quantities of 30 the Tap plastics parts are under $1.50 with tax and shipping.

To provide strong attachment for the lid bolts you will add brass inserts to the lid. [Here is one option[(https://www.amazon.com/gp/product/B08BCRZZS3/).

You will also need some o-ring material for the groove in the lid. I have tried [one from Amazon which works](https://www.amazon.com/gp/product/B00QVB6NE4)
and [another I like better](https://www.amazon.com/Actual-Silicone-Durometer-Thickness-10/dp/B00QVB13P8).  Either gives you plenty of material for more boxes than most labs ever need.

The overlap files are optional, but I prefer to use them for making strong boxes with less material.
The idea is to allow the slicer to hollow the walls, but not at the corners and around key connectors.
Most slicing software will support this in some form, but here is a rough outline of how it works in
CURA 5.7.
- Import one box file and a matching overlap file.
- Ensure that their XY positions are identical so they overlap perfectly.
- Select the matching file in CURA
- Click the Support Blocker icon at the left
- Click Per model settings and choose modify settings for overlaps at the top
- Set the wall thicknessa and top/bottom thickness to 3 or more
- Choose your preferred slicing settings.  I use 0.84 mm walls and top with an 0.42 mm bottom.  Cubic subdivision infill at 20% works for me.

Again, your equipment and expertise should be applied appropriately.  There are dozens or hundreds
of possible settings.

I do have some observations on materials
- PLA is common, but is the most flammable and most likely to melt by accident.
- PETG is more robust and may be available as a recycled material.
- ABS is sturdy and the most UV-resistant of the set.

I plan to spray my boxes with a UV-resistant clearcoat such as Krylon K01309.  Again, take your needs into account.

No printer access?  There are many vendors online who can do this.  You can also ask me if I have time to do it.  Perhaps you can send me a spool of PETG or ABS in your favorite color, and I'll keep the leftover material as payment.
