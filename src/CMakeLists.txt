add_executable (
  sudodevd
  sudodevd.c
  chomp.c
  color.c
  daemonize.c
  devs.c
  find.c
  group.c
  lockfile.c
  readfile.c
  say.c
  sort.c
  sudoers.c
)
target_link_libraries (sudodevd pthread)

add_executable (
  sudodev
  sudodev.c
  chomp.c
  color.c
  devs.c
  find.c
  profile.c
  readfile.c
  say.c
  sort.c
)
target_link_libraries (sudodev pthread)

