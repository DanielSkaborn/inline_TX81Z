# inline_TX81Z

This program replace MIDI cc messages with Yamaha TX81Z parameter change messages.
The MIDI cc mapping is made in the mapping file.
Row 1 is midi device (in you Linux system)
Row 2 is midi channel
Row 3 is number of pages of parameters per MIDI cc
Then comes a list of the mapping (matrix) where first column is cc, and next columns are TX parameter numbers.
Every 2nd row is max value for the parameter.
One parameter is used as store to syx file. All parameters has to be edited to perform a store.
In the matrix, all number are HEX.

See source for further documenation.
