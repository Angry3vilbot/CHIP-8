# CHIP-8 Emulator
A Standard CHIP-8 Emulator written in C++. Uses SDL3 for keyboard input and drawing to a window.
## How to run
1. Download the release and your ROM(s) of choice.
   
    > [!IMPORTANT]  
    > Only supports base CHIP-8 ROMs. Does NOT support SUPER-CHIP or XO-CHIP.
2. Extract the release somewhere.
3. Run it via the terminal using the command:
   ```./CHIP8.exe <Scale> <Delay> <ROM>```
   , where:
   - `<Scale>` is the scale factor to which to scale the display (CHIP8's base display resolution is only 64x32 pixels)
   - `<Delay>` is the delay between each CPU cycle. Most games will require you to set it higher than 1, otherwise they'd be unplayable due to the speed.
   - `<ROM>` is the filename of/path to the ROM that you wish to load.
## Building from source
To run from the source code, you will need to download and link/include the SDL3 library to the project. Once that is done, placing the .dll of the library in the directory where the debug/build output is located should be enough to make it run. There are guides on how to do this, I followed [this one](https://lazyfoo.net/SDL_tutorials/lesson01/windows/msvsnet2010e/index.php).