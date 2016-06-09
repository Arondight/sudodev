set (INIT_SCRIPT_PATH ${CMAKE_CURRENT_LIST_DIR})
set (SYSTEMD_DIR /lib/systemd/system)
set (UPSTART_DIR /etc/init)
set (SYSVINIT_DIR /etc/init.d)
set (BSDINIT_DIR /etc/rc.d)
set (SYSTEMD_SCRIPT ${INIT_SCRIPT_PATH}/systemd/sudodevd.service)
set (UPSTART_SCRIPT ${INIT_SCRIPT_PATH}/upstart/sudodevd.conf)
set (SYSVINIT_SCRIPT ${INIT_SCRIPT_PATH}/sysv/sudodevd)
set (BSDINIT_SCRIPT ${INIT_SCRIPT_PATH}/bsd/rc.sudodevd)

if (IS_DIRECTORY ${SYSTEMD_DIR})
  execute_process (COMMAND install -Dm0644 ${SYSTEMD_SCRIPT} ${SYSTEMD_DIR})
  execute_process (COMMAND systemctl daemon-reload)
  execute_process (COMMAND systemctl enable sudodevd)
  execute_process (COMMAND systemctl start sudodevd)
elseif (IS_DIRECTORY ${UPSTART_DIR})
  execute_process (COMMAND install -Dm0644 ${UPSTART_SCRIPT} ${UPSTART_DIR})
  execute_process (COMMAND initctl reload-configuration)
  execute_process (COMMAND initctl start sudodevd)
elseif (IS_DIRECTORY ${SYSVINIT_DIR})
  execute_process (COMMAND install -Dm0644 ${SYSVINIT_SCRIPT} ${SYSVINIT_DIR})
  execute_process (COMMAND initctl reload-configuration)
  foreach (level RANGE 2 5)
    execute_process (COMMAND ln -s ${SYSVINIT_DIR}/sudodevd /etc/rc${level}.d/S60sudodevd)
  endforeach ()
  foreach (level 0 1 6)
    execute_process (COMMAND ln -s ${SYSVINIT_DIR}/sudodevd /etc/rc${level}.d/K60sudodevd)
  endforeach ()
  execute_process (COMMAND chmod 755 ${SYSVINIT_DIR}/sudodevd)
  execute_process (COMMAND ${SYSVINIT_DIR}/sudodevd start)
elseif (IS_DIRECTORY ${BSDINIT_DIR})
  execute_process (COMMAND install -Dm0644 ${BSDINIT_SCRIPT} ${BSDINIT_DIR})
  message (STATUS "It seems you are using BSD init.")
  message (STATUS "You have to write lines in suitable location of files manually to set start/stop runlevels:")
  message (STATUS "")
  message (STATUS "For ${BSDINIT_DIR}/rc.4 and ${BSDINIT_DIR}/rc.M:")
  message (STATUS "[ -x ${BSDINIT_DIR}/rc.sudodevd ] && ${BSDINIT_DIR}/rc.sudodevd start")
  message (STATUS "")
  message (STATUS "For ${BSDINIT_DIR}/rc.0, ${BSDINIT_DIR}/rc.6 and ${BSDINIT_DIR}/rc.K:")
  message (STATUS "[ -x ${BSDINIT_DIR}/rc.sudodevd ] && ${BSDINIT_DIR}/rc.sudodevd stop")
  message (STATUS "")
  execute_process (COMMAND chmod 755 ${BSDINIT_DIR}/rc.sudodevd)
  execute_process (COMMANDsudo ${BSDINIT_DIR}/rc.sudodevd start)
endif ()

execute_process (COMMAND gpasswd -a $ENV{USER} sudodev)
