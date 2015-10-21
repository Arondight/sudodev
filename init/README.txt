This directory is init scripts for systemd, Upstart, SysVinit and BSD init.

CMakelist.txt will not deal this directory, because there is no common way to
indentify which init program you are using (not only installed), no matter via
"/proc/1/exe", "/dev/initctl" or other way. So you should do that manually.

Copy corresponding one to target directory:

  ---------------------------------------------------------------------------
  | init program    | script file                 | target directory        |
  ---------------------------------------------------------------------------
  | systemd         | ./systemd/sudodevd.service  | /lib/systemd/system/    |
  | Upstart         | ./upstart/sudodevd.conf     | /etc/init/              |
  | SysVinit        | ./sysv/sudodevd             | /etc/init.d/            |
  | BSD init        | ./bsd/rc.sudodevd           | /etc/rc.d/              |
  ---------------------------------------------------------------------------

If you are new to linux world, here is what you should do accroding to your
init program is:

  1. systemd

    sudo systemctl daemon-reload
    sudo systemctl enable sudodevd
    sudo systemctl start sudodevd

  2. Upstart

    sudo initctl reload-configuration
    sudo initctl start sudodevd

  3. SysVinit

    echo -n /etc/rc{2,3,4,5}.d/S60sudodevd | \
      xargs -d ' ' -I{} sudo ln -s /etc/init.d/sudodevd {}
    echo -n /etc/rc{0,1,6}.d/K60sudodevd | \
      xargs -d ' ' -I{} sudo ln -s /etc/init.d/sudodevd {}
    sudo chmod 755 /etc/init.d/sudodevd
    sudo /etc/init.d/sudodevd start

  4. BSD init

    sudo sh -c 'echo sudodevd=YES >> /etc/rc.conf'
    sudo chmod 755 /etc/rc.d/rc.sudodevd
    sudo /etc/rc.d/rc.sudodevd start

