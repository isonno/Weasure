Weasure UPS Demo
================

This is a simple Python program to use the weasure to automatically get quotes from UPS.  It was built to demonstrate
the [Weasure](http://www.weasure.com/) scale.

When you run the script, it should open a web browser.  Fill in the city, zip and value (insurance)
for your package.  When you hit the "Get Rate" button, it'll look up the price on the UPS web site.

Note by default, the script does NOT use Weasure.  If you do actually have the scale connected, you'll need to
delete the `return` in `UpdateMeasurements`, and make sure Weasure is connected to your serial port.

The UPS API is documented in the [UPS Rates and Services Online guide](https://www.ups.com/gec/techdocs/pdf/RatesandServiceHTML.pdf).

**Note** This was implemented in Python 2 many years ago. There's no guarentee this code still runs or UPS still supports the same web APIs.
