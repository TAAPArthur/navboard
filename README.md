# navboard

Navboard is a general purpose application designed to translate touch/mouse events into arbitrary actions. This makes it ideal as a on-screen reader (OSK) but it can also server as a general application launcher.

## Install

```
make;
make install
```
Note that default settings can be customized by modifying config.h/config.c

## Dependencies

The following libraries are needed to build
- Standard X libs: -lX11 -lxcb
- X WM helper libs: -lxcb-ewmh -lxcb-icccm
- Lib to send fake key/button events: Library-lxcb-xtest
- Font related libs: -lxcb-render -lxcb-render-util -lfreetype

For tests, [scunittest](https://codeberg.org/TAAPArthur/scunittest) is also needed.

## Usage

`navboard` opens up a launcher to select the board to use. The default action can be changed by setting NAVBOARD_DEFAULT env variable
`navboard board_of_boards` same as the above
`navboard qwerty ` opens up the default OSK (standard qwerty layout)
`navboard mobile` opens up the default mobile OSK

## Custom boards

navboard is installed system wide but users can each have their own boards.
To add your own board add the c source file to ${XDG_CONFIG_DIR:-$HOME/.config}/navboard/
and then run `navboard -r` to recompile a local copy for the given user. Future
runs of navboard will show all custom boards defined for the user.

### Dynamic Custom boards
Any excutable file ending in `*.sh` that is in ${XDG_CONFIG_DIR:-$HOME/.config}/navboard/ will be run before recompliatoin. This can be used to create boards based on the layout of the running system.

For a real example see [volume.h](https://codeberg.org/TAAPArthur/SystemConfig/src/branch/master/Config/.config/navboard/volume.h) and [volume_board_gen.sh](https://codeberg.org/TAAPArthur/SystemConfig/src/branch/master/Config/.config/navboard/volume_board_gen.sh). These scripts work to getter to create a board based on the existing alsa devices and server to provide graphical volume controller. The neat part is I can use the same script on all my devices.

## Sample boards
In [samples/](samples/), there are some toy boards to demenstrate some functionality that is beyond a typical OSK. However to avoid overcomplicating them, they don't actually do anything.
build with `SAMPLES=1` to include these dummy examples

