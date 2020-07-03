Usage: mcufont <command> [options] ...
Commands for importing:
   import_ttf <ttffile> <size> [bw]     Import a .ttf font into a data file.
   import_bdf <bdffile>                 Import a .bdf font into a data file.

Commands for inspecting and editing data files:
   filter <datfile> <range> ...         Remove everything except specified characters.
   show_glyph <datfile> <index>         Show the glyph at index.

Commands specific to rlefont format:
   rlefont_size <datfile>               Check the encoded size of the data file.
   rlefont_optimize <datfile>           Perform an optimization pass on the data file.
   rlefont_export <datfile> [outfile]   Export to .c source code.
   rlefont_show_encoded <datfile>       Show the encoded data for debugging.

Commands specific to bwfont format:
   bwfont_export <datfile> [outfile     Export to .c source code.

