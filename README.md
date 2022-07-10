# dwmblocks-async
A modular statusbar for `dwm` written in C. You may think of it as `i3blocks`, but for `dwm`.

![A lean config of dwmblocks-async.](preview.png)

## Features
- Modular
- Lightweight
- Suckless
- Blocks are clickable
- Blocks are loaded asynchronously
- Each block can be externally triggered to update itself
- Compatible with `i3blocks` scripts

> Apart from these features, this build of `dwmblocks` is more optimized and fixes the scroll issue due to which the statusbar flickers on scrolling.

## Why `dwmblocks`?
In `dwm`, you have to set the statusbar through an infinite loop like this:

```sh
while :; do
    xsetroot -name "$(date)"
    sleep 30
done
```

It may not look bad as it is, but it's surely not the most efficient way when you've got to run multiple commands, out of which only few need to be updated as frequently as the others. 

```sh
# Displaying an unread mail count in the status bar
while :; do
    xsetroot -name "$(mailCount) $(date)"
    sleep 60
done
```

For example, I display an unread mail count in my statusbar. Ideally, I would want this count to update every thirty minutes, but since I also have a clock in my statusbar which has to be updated every minute, I can't stop the mail count from being updated every minute.

As you can see, this is wasteful. And since my mail count script uses Gmail's APIs, there's a limit to the number of requests I can make, being a free user.  

What `dwmblocks` does is that it allows you to break up the statusbar into multiple blocks, each of which have their own update interval. The commands in a particular block are only executed once in that interval. Hence, we don't run into our problem anymore.

What's even better is that you can externally trigger updation of any specific block.


## Why `dwmblocks-async`?
Everything I have mentioned till now is offered by the vanilla `dwmblocks`, which is fine for most users. What sets `dwmblocks-async` apart from vanilla `dwmblocks` is the 'async' part. `dwmblocks` executes the commands of each blocks sequentially which means that the mail and date blocks, from above example, would be executed one after the other. This means that the date block won't update unless the mail block is done executing, or vice versa. This is bad for scenarios where one of the blocks takes seconds to execute, and is clearly visible when you first start `dwmblocks`.

This is where the async nature of `dwmblocks-async` steps in tells the computer to execute each block asynchronously or simultaneously.


## Installation
The installation is simple, just clone this repository, modify `config.h` appropriately, and then do a `sudo make install`.

```sh
git clone https://github.com/UtkarshVerma/dwmblocks-async.git
vi config.h
sudo make install
```


## Usage
To have `dwmblocks-async` set your statusbar, you need to run it as a background process on startup. One way is by adding the following to `~/.xinitrc`.

```sh
# `dwmblocks-async` has its binary named `dwmblocks`
dwmblocks &
```

### Modifying the blocks
You can define your statusbar blocks in `config.h`. Each block has the following properties:

Property|Value
-|-
Command | The command you wish to execute in your block
Update interval | Time in seconds, after which you want the block to update. Setting this to `0` will result in the block never being updated.
Update signal | Signal to be used for triggering the block. Must be a positive integer. If the value is `0`, signal won't be set up for the block and it will be unclickable.

The syntax for defining a block is:
```c
const Block blocks[] = {
    ...
    BLOCK("volume", 0,    5),
    BLOCK("date",   1800, 1),
    ...
}
```

Apart from that you can also modify the following parameters to suit your needs.
```c
// Maximum possible length of output from block, expressed in number of characters.
#define CMDLENGTH 50

// The status bar's delimiter which appears in between each block.
#define DELIMITER " "

// Adds a leading delimiter to the statusbar, useful for powerline.
#define LEADING_DELIMITER

// Enable clickability for blocks. Needs `dwm` to be patched appropriately.
// See the "Clickable blocks" section below.
#define CLICKABLE_BLOCKS
```

### Signalling changes
Most statusbars constantly rerun every script every several seconds to update. This is an option here, but a superior choice is giving your block a signal that you can signal to it to update on a relevant event, rather than having it rerun idly.

For example, the volume block has the update signal 5 by default.  Thus, running `pkill -RTMIN+5 dwmblocks` will update it.

You can also run `kill -39 $(pidof dwmblocks)` which will have the same effect, but is faster. Just add 34 to your typical signal number.

My volume block *never* updates on its own, instead I have this command run along side my volume shortcuts in `dwm` to only update it when relevant.

Note that all blocks must have different signal numbers.

Apart from this, you can also refresh all the blocks by sending `SIGUSR1` to `dwmblocks-async` using either `pkill -SIGUSR1 dwmblocks` or `kill -10 $(pidof dwmblocks)`.

### Clickable blocks
Like `i3blocks`, this build allows you to build in additional actions into your scripts in response to click events. You can check out [my statusbar scripts](https://github.com/UtkarshVerma/dotfiles/tree/main/.local/bin/statusbar) as references for using the `$BLOCK_BUTTON` variable.

To use this feature, define the `CLICKABLE_BLOCKS` feature macro in your `config.h`.
```c
#define CLICKABLE_BLOCKS
```

Apart from that, you need `dwm` to be patched with [statuscmd](https://dwm.suckless.org/patches/statuscmd/).


## Credits
This work would not have been possible without [Luke's build of dwmblocks](https://github.com/LukeSmithxyz/dwmblocks) and [Daniel Bylinka's statuscmd patch](https://dwm.suckless.org/patches/statuscmd/).
