This is what CMakelist.txt in this direcotry will do.

At the very beginning cmake try to copy corresponding files to target directory:

  ---------------------------------------------------------------------------
  | init program    | script file                 | target directory        |
  ---------------------------------------------------------------------------
  | systemd         | ./systemd/sudodevd.service  | /lib/systemd/system/    |
  | Upstart         | ./upstart/sudodevd.conf     | /etc/init/              |
  | SysVinit        | ./sysv/sudodevd             | /etc/init.d/            |
  | BSD init        | ./bsd/rc.sudodevd           | /etc/rc.d/              |
  ---------------------------------------------------------------------------

Then cmake will do something accroding to your init program is:

  1. systemd (for example, Arch Linux, Ubuntu 14.X+, Debian 6.0+, openSUSE 11.4+
              Fedora 16+, CentOS 6.X+ and Red Hat Enterprise Linux 6.X+)

    Reload daemons config:
      systemctl daemon-reload

    Set start targets:
      systemctl enable sudodevd

    Start it now:
      systemctl start sudodevd

  2. Upstart (for example, ealier versions of Ubuntu and Debian)

    Reload daemons config:
      initctl reload-configuration

    Start it now:
      initctl start sudodevd

    Start/stop runlevels has been set on sudodevd.conf, it is no need to set
    autostart manually.

  3. SysVinit (for example, ealier versions of Fedora, CentOS and RHEL)

    Set start/stop runlevels:
      echo -n /etc/rc{2,3,4,5}.d/S60sudodevd | \
        xargs -d ' ' -I{} ln -s /etc/init.d/sudodevd {}
      echo -n /etc/rc{0,1,6}.d/K60sudodevd | \
        xargs -d ' ' -I{} ln -s /etc/init.d/sudodevd {}

    Set to executable:
      chmod 755 /etc/init.d/sudodevd

    Start it now:
      /etc/init.d/sudodevd start

  4. BSD init (for exmaple, Slackware Linux)

    For BSD init, it is a bit complicated, you can not configure these by
    conmands. You have to write lines in suitable location of files manually
    to set start/stop runlevels:

    For /etc/rc.d/rc.4 and /etc/rc.d/rc.M:
      [ -x /etc/rc.d/rc.sudodevd ] && /etc/rc.d/rc.sudodevd start

    For /etc/rc.d/rc.0, /etc/rc.d/rc.6 and /etc/rc.d/rc.K:
      [ -x /etc/rc.d/rc.sudodevd ] && /etc/rc.d/rc.sudodevd stop

    Set to executable:
      chmod 755 /etc/rc.d/rc.sudodevd

    Start it now:
      /etc/rc.d/rc.sudodevd start

At last cmake try to add current user to sudodev group:

  gpasswd -a $USER sudodev

