#! /bin/sh
export EFI_BOOT_MEDIA=./data.img
echo "Start to create file boot disk ..."
dd bs=512 count=2880 if=/dev/zero of=$EFI_BOOT_MEDIA
mkfs.msdos -F 12 $EFI_BOOT_MEDIA
mcopy -i $EFI_BOOT_MEDIA ./name.txt ::/name.txt
mmd -i $EFI_BOOT_MEDIA ::/efi ::/efi/boot
mcopy -i $EFI_BOOT_MEDIA ./Shell_Full.efi ::/efi/boot/BootX64.efi
mdir -i $EFI_BOOT_MEDIA -s ::
./GenBootSector -i $EFI_BOOT_MEDIA -o $EFI_BOOT_MEDIA.bs0
cp ./bootsect.com $EFI_BOOT_MEDIA.bs1
./BootSectImage -g $EFI_BOOT_MEDIA.bs0 $EFI_BOOT_MEDIA.bs1
./GenBootSector -o $EFI_BOOT_MEDIA -i $EFI_BOOT_MEDIA.bs1
rm $EFI_BOOT_MEDIA.bs[0-1]
rm IMG.c
xxd -i $EFI_BOOT_MEDIA > IMG.c

echo Done.

