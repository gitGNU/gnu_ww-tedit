synh.exe: synh.obj
  cl -Zi synh.obj

# add -DTEST to have main()
synh.obj: synh.c
  cl -I.. -Zi -DTEST -c synh.c

synh.c: synh.l
  flex -osynh.c synh.l
