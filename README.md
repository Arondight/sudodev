# NAME

sudodev - Create your special devices for sudo privilege without password

# SYNOPSIS

This allows you to choose some devices (identify by [UUID][ID_UUID]),so that when any of
them are connected to your computer, your sudo command no longer need password.

[ID_UUID]: https://en.wikipedia.org/w/index.php?title=Universally_unique_identifier "Learn more about UUID"

# INSTALL

```shell
git clone https://github.com/Arondight/sudodev.git sudodev
cd sudodev
```

Goto [INSTALL][ID_INSTALL] for installation instructions.

Read [init/README.txt][ID_INIT_README_TXT] for how to configure the inti script.

[ID_INSTALL]: INSTALL "Read INSTALL"
[ID_INIT_README_TXT]: init/README.txt "Read init/README.txt"

# USAGE

1. `sudodev`

    This allows you to add or to delete devices

2. `sudodevd`

    This is the daemon, you should not start this manually, you are recommanded to 
start it via your sevice manager such as systemctl and initctl (for earlier,
`/etc/init.d/sudodevd` for SysVinit and `/etc/rc.d/rc.sudodevd` for BSD init).

# SECURITY

Do not worry, this is secure.

In addition, you can remove rules for your account from `/etc/sudoers` after creating
a "special device", so that only people in the following two categories can get root
privilege:

1. He knows root password (if your root account has a password)
+ He has one of these special devices

# NOTICE

**Do not repart your device**, or UUID will change, and this device
will lose effectiveness.

# COPYRIGHT

Copyright (c) 2015 秦凡东 (Qin Fandong)

# LICENSE

See [LICENSE][ID_LICENSE]

[ID_LICENSE]: LICENSE "Read LICENSE"

