# dwmblocks-async

A [`dwm`](https://dwm.suckless.org) status bar that has a modular, async
design, so it is always responsive. Imagine `i3blocks`, but for `dwm`.

![A lean config of dwmblocks-async.](preview.png)

## Features

- [Modular](#modifying-the-blocks)
- Lightweight
- [Suckless](https://suckless.org/philosophy)
- Blocks:
  - [Clickable](#clickable-blocks)
  - Loaded asynchronously
  - [Updates can be externally triggered](#signalling-changes)
- Compatible with `i3blocks` scripts

> Additionally, this build of `dwmblocks` is more optimized and fixes the
> flickering of the status bar when scrolling.

## Why `dwmblocks`?

In `dwm`, you have to set the status bar through an infinite loop, like so:

```sh
while :; do
    xsetroot -name "$(date)"
    sleep 30
done
```

This is inefficient when running multiple commands that need to be updated at
different frequencies. For example, to display an unread mail count and a clock
in the status bar:

```sh
while :; do
    xsetroot -name "$(mailCount) $(date)"
    sleep 60
done
```

Both are executed at the same rate, which is wasteful. Ideally, the mail
counter would be updated every thirty minutes, since there's a limit to the
number of requests I can make using Gmail's APIs (as a free user).

`dwmblocks` allows you to divide the status bar into multiple blocks, each of
which can be updated at its own interval. This effectively addresses the
previous issue, because the commands in a block are only executed once within
that time frame.

## Why `dwmblocks-async`?

The magic of `dwmblocks-async` is in the `async` part. Since vanilla
`dwmblocks` executes the commands of each block sequentially, it leads to
annoying freezes. In cases where one block takes several seconds to execute,
like in the mail and date blocks example from above, the delay is clearly
visible. Fire up a new instance of `dwmblocks` and you'll see!

With `dwmblocks-async`, the computer executes each block asynchronously
(simultaneously).

## Installation

Clone this repository, modify `config.h` appropriately, then compile the
program:

```sh
git clone https://github.com/UtkarshVerma/dwmblocks-async.git
cd dwmblocks-async
vi config.h
sudo make install
```

## Usage

To set `dwmblocks-async` as your status bar, you need to run it as a background
process on startup. One way is to add the following to `~/.xinitrc`:

```sh
# The binary of `dwmblocks-async` is named `dwmblocks`
dwmblocks &
```

### Modifying the blocks

You can define your status bar blocks in `config.c`:

```c
Block blocks[] = {
    ...
    {"volume", 0,    5},
    {"date",   1800, 1},
    ...
}
```

Each block has the following properties:

| Property        | Description                                                                                                                                        |
| --------------- | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| Command         | The command you wish to execute in your block.                                                                                                     |
| Update interval | Time in seconds, after which you want the block to update. If `0`, the block will never be updated.                                                |
| Update signal   | Signal to be used for triggering the block. Must be a positive integer. If `0`, a signal won't be set up for the block and it will be unclickable. |

Apart from defining the blocks, features can be toggled through `config.h`:

```c
// Maximum possible length of output from block, expressed in number of characters.
#define CMDLENGTH 50

// The status bar's delimiter that appears in between each block.
#define DELIMITER " "

// Adds a leading delimiter to the status bar, useful for powerline.
#define LEADING_DELIMITER 1

// Enable clickability for blocks. See the "Clickable blocks" section below.
#define CLICKABLE_BLOCKS 1
```

### Signalling changes

Most status bars constantly rerun all scripts every few seconds. This is an
option here, but a superior choice is to give your block a signal through which
you can indicate it to update on relevant event, rather than have it rerun
idly.

For example, the volume block has the update signal `5` by default. I run
`kill -39 $(pidof dwmblocks)` alongside my volume shortcuts in `dwm` to only
update it when relevant. Just add `34` to your signal number! You could also
run `pkill -RTMIN+5 dwmblocks`, but it's slower.

To refresh all the blocks, run `kill -10 $(pidof dwmblocks)` or
`pkill -SIGUSR1 dwmblocks`.

> All blocks must have different signal numbers!

### Clickable blocks

Like `i3blocks`, this build allows you to build in additional actions into your
scripts in response to click events. You can check out
[my status bar scripts](https://github.com/UtkarshVerma/dotfiles/tree/main/.local/bin/statusbar)
as references for using the `$BLOCK_BUTTON` variable.

To use this feature, define the `CLICKABLE_BLOCKS` feature macro in your
`config.h`:

```c
#define CLICKABLE_BLOCKS 1
```

Apart from that, you need `dwm` to be patched with
[statuscmd](https://dwm.suckless.org/patches/statuscmd/).

## Credits

This work would not have been possible without
[Luke's build of dwmblocks](https://github.com/LukeSmithxyz/dwmblocks) and
[Daniel Bylinka's statuscmd patch](https://dwm.suckless.org/patches/statuscmd/).
