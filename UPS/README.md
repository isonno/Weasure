Weasure UPS Demo
================

This is a simple Python program to use the weasure to automatically get quotes from UPS.  

When you run the script, it should open a web browser.  Fill in the city, zip and value (insurance)
for your package.  When you hit the "GetRate" button, it'll look up the price on the UPS web site.

Note by default, the script does NOT use Weasure.  If you do actually have the scale connected, you'll need to
delete the `return` in `UpdateMeasurements`, and make sure Weasure is connected to your serial port.

The UPS API is documented in the [UPS Rates and Services Online guide](https://www.ups.com/gec/techdocs/pdf/RatesandServiceHTML.pdf).
