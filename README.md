# NAME

sudodev - Create your special device for a none-password sudo

# SYNOPSIS

This let you choose some devices (identify by [UUID][ID_UUID]), when any of them
connect to your computer, your sudo command no longer need password.

You can use `sudodev` command to add/delete device.

[ID_UUID]: https://en.wikipedia.org/w/index.php?title=Universally_unique_identifier "Learn more about UUID"

# INSTALL

```shell
git clone https://github.com/Arondight/sudodev.git sudodev
cd sudodev
```

See [INSTALL][ID_INSTALL] to install program.

See [init/README.txt][ID_INIT_README_TXT] to config init script.

[ID_INSTALL]: INSTALL "Read INSTALL"
[ID_INIT_README_TXT]: init/README.txt "Read init/README.txt"

# USAGE

1. `sudodev`

    This let you add/delete device

2. `sudodevd`

    This is the daemon, you should not startup this manually, you had better
start it via your sevice manager such as systemctl and initctl (to older,
`/etc/init.d/sudodevd` for SysVinit and `/etc/rc.d/rc.sudodevd` for BSD init).

# SAFETY

Do not worry, this is safe in most cases.

And for more, you can delete rule of your account in `/etc/sudoers` after create
a "special device", then only people with the following two cases can get root
privilege:

1. He know root password (if your root account has a password)
2. He have one of these special devices

What you should not do is add your local device using `sudodev`.
**if you do that, your sudo command no longer need password in any case**.

# NOTICE

Do not repart your device, or UUID will changed, and this device
will lose effectiveness.

# COPYRIGHT

Copyright (c) 2015 秦凡东 (Qin Fandong)

# LICENSE

See [LICENSE][ID_LICENSE]

[ID_LICENSE]: LICENSE "Read LICENSE"

