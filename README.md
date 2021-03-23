# dwmblocks
A modular statusbar for `dwm` written in C. You may think of it as `i3blocks`, but for `dwm`.

## Features
- Modular
- Lightweight
- Suckless
- Modules are clickable
- Each module can be externally triggered to update itself
- Compatible with `i3blocks` scripts

> Apart from these features, this build of `dwmblocks` is more optimized and fixes the scroll issue due to which the statusbar flickers on scrolling.

# Why `dwmblocks`?
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


# Installation
The installation is simple, just clone this repository, modify `config.h` appropriately, and then do a `sudo make install`.

```sh
git clone https://github.com/UtkarshVerma/dwmblocks.git
vi config.h
sudo make install
```


# Usage
To have `dwmblocks` set your statusbar, you need to run it as a background process on startup. One way is by adding the following to `~/.xinitrc`.

```sh
dwmblocks &
```

## Modifying the blocks
You can define your statusbar blocks in `config.h`. Each block has the following properties:

Property|Value
-|-
Command | The command you wish to execute in your block
Update interval | Time in seconds, after which you want the block to update. Setting this to `0` will result in the block never being updated.
Update signal | Signal to be used for triggering the block. Must be a positive integer. 

The syntax for defining a block is:
```c
static const Block blocks[] = {
    ...
    {"date", 1800, 1},
    ...
}
```

Apart from that you can also modify the block delimiters and width of each block as shown.
```c
// Maximum length of each block, expressed in number of characters.
#define CMDLENGTH 50

#define delimiter " "
```

## Signalling changes
Most statusbars constantly rerun every script every several seconds to update. This is an option here, but a superior choice is giving your module a signal that you can signal to it to update on a relevant event, rather than having it rerun idly.

For example, the volume module has the update signal 5 by default.  Thus, running `pkill -RTMIN+5 dwmblocks` will update it.

You can also run `kill -44 $(pidof dwmblocks)` which will have the same effect, but is faster. Just add 34 to your typical signal number.

My volume module *never* updates on its own, instead I have this command run along side my volume shortcuts in `dwm` to only update it when relevant.

Note that all modules must have different signal numbers.

## Clickable modules
Like `i3blocks`, this build allows you to build in additional actions into your scripts in response to click events. You can check out [my statusbar scripts](https://github.com/UtkarshVerma/dotfiles/tree/gentoo/.local/bin/statusbar) as references for using the `$BLOCK_BUTTON` variable.

For this feature to work, you need `dwm` to be patched with [statuscmd](https://dwm.suckless.org/patches/statuscmd/).

# Credits
This work would not have been possible without [Luke's build of dwmblocks](https://github.com/LukeSmithxyz/dwmblocks) and [Daniel Bylinka's statuscmd patch](https://dwm.suckless.org/patches/statuscmd/).
