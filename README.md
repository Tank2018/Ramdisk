# Ramdisk
 a ramdisk under uefi

1.run CreateImg.sh to gen the IMG.c
2.use "mcopy -i $EFI_BOOT_MEDIA ./1.txt ::/1.txt" in CreateImg.sh to 
  add file in ramdisk

