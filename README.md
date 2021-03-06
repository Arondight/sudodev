# sudodev

## ABOUT

This allows you to choose some devices (identify by Partition's [UUID][ID_UUID]), so that when any of them are connected to your computer, your sudo command no longer need password.

[ID_UUID]: https://en.wikipedia.org/w/index.php?title=Universally_unique_identifier "Learn more about UUID"

## INSTALLATION

### ARCH LINUX

```bash
yaourt -S sudodev-git
```

### OTHER DISTRIBUTIONS

First install sudodev.

```bash
git clone https://github.com/Arondight/sudodev.git
cd sudodev
./install.sh
```

Then reboot to accept new group.

```bash
sudo reboot
```

## USAGE

```bash
sudodev help
```

## NOTICE

Do **NOT** repart your device or it will lose effectiveness.

## COPYRIGHT

Copyright (c) 2015-2016 秦凡东 (Qin Fandong)

## LICENSE

Read [LICENSE][ID_LICENSE]

[ID_LICENSE]: LICENSE "Read LICENSE"

