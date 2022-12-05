# dwmblocks-async
An async, modular statusbar for `dwm`. You may think of it as `i3blocks`, but for `dwm`.

![A lean config of dwmblocks-async.](preview.png)

## Features
- Modular
- Lightweight
- Suckless
- Blocks are clickable
- Blocks are loaded asynchronously
- Each block can be externally triggered to update itself
- Compatible with `i3blocks` scripts

> Additionally, this build of `dwmblocks` is more optimized and fixes the flickering of the statusbar when scrolling.

## Why `dwmblocks`?
In `dwm`, you have to set the statusbar through an infinite loop like this:
```sh
while :; do
    xsetroot -name "$(date)"
    sleep 30
done
```

This is inefficient when running multiple commands that need to be updated at different frequencies. For example, displaying an unread mail count and a clock in the statusbar:
```sh
while :; do
    xsetroot -name "$(mailCount) $(date)"
    sleep 60
done
```

Both are executed at the same rate, which is wasteful. Ideally, the mail counter would be updated every thirty minutes, since there's a limit to the number of requests I can make using Gmail's APIs (as a free user).  

`dwmblocks` allows you to break up the statusbar into multiple blocks, each having their own update interval. The commands in a particular block are only executed once in that interval, solving our previous problem.

> Additionally, you can externally trigger any specific block.

## Why `dwmblocks-async`?
The magic of `dwmblocks-async` is in the `async` part. Since vanilla `dwmblocks` executes the commands of each block sequentially, it leads to annoying freezes. In cases where one block takes several seconds to execute, like in the mail and date blocks example from above, the delay is clearly visible. Fire up a new instance of `dwmblocks` and you'll see!

With `async`, the computer executes each block asynchronously (simultaneously).

## Installation
Clone this repository, modify `config.h` appropriately, then compile the program:
```sh
git clone https://github.com/UtkarshVerma/dwmblocks-async.git
cd dwmblocks-async
vi config.h
sudo make install
```

## Usage
To set `dwmblocks-async` as your statusbar, you need to run it as a background process on startup. One way is to add the following to `~/.xinitrc`:

```sh
# Binary of `dwmblocks-async` is named `dwmblocks`
dwmblocks &
```

### Modifying the blocks
You can define your statusbar blocks in `config.h`: 
```c
const Block blocks[] = {
    ...
    BLOCK("volume", 0,    5),
    BLOCK("date",   1800, 1),
    ...
}
```

Each block has the following properties:

Property|Description
-|-
Command | The command you wish to execute in your block
Update interval | Time in seconds, after which you want the block to update. If `0`, the block will never be updated.
Update signal | Signal to be used for triggering the block. Must be a positive integer. If `0`, a signal won't be set up for the block and it will be unclickable.

Additional parameters can be modified:

```c
// Maximum possible length of output from block, expressed in number of characters.
#define CMDLENGTH 50

// The statusbar's delimiter that appears in between each block.
#define DELIMITER " "

// Adds a leading delimiter to the statusbar, useful for powerline.
#define LEADING_DELIMITER

// Enable clickability for blocks. See the "Clickable blocks" section below.
#define CLICKABLE_BLOCKS
```

### Signalling changes
Most statusbars constantly rerun all scripts every few seconds. This is an option here, but a superior choice is to give your block a signal through which you can indicate it to update on relevant event, rather than have it rerun idly.

For example, the volume block has the update signal 5 by default:

Command|Effect
-|-
`pkill -RTMIN+5 dwmblocks`|Update block
`kill -39 $(pidof dwmblocks)`|Update, but faster. Add 34 to your signal number!
`pkill -SIGUSR1 dwmblocks`|Refresh all blocks
`kill -10 $(pidof dwmblocks)`|Refresh, but faster

My volume block *never* updates on its own. Instead, I use the 2nd command alongside my volume shortcuts in `dwm` to only update it when relevant.

> All blocks must have different signal numbers!

### Clickable blocks
Like `i3blocks`, this build allows you to build in additional actions into your scripts in response to click events. You can check out [my statusbar scripts](https://github.com/UtkarshVerma/dotfiles/tree/main/.local/bin/statusbar) as references for using the `$BLOCK_BUTTON` variable.

To use this feature, define the `CLICKABLE_BLOCKS` feature macro in your `config.h`:

```c
#define CLICKABLE_BLOCKS
```

Apart from that, you need `dwm` to be patched with [statuscmd](https://dwm.suckless.org/patches/statuscmd/).


## Credits
This work would not have been possible without [Luke's build of dwmblocks](https://github.com/LukeSmithxyz/dwmblocks) and [Daniel Bylinka's statuscmd patch](https://dwm.suckless.org/patches/statuscmd/).
