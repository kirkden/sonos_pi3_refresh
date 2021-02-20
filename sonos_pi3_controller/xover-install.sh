DESTDIR=/mnt/mmcblk0p2/tce/optional

install -m644 sonos-pi3-plugins.tcz $DESTDIR/
grep -qxF 'sonos-pi3-plugins.tcz' /mnt/mmcblk0p2/tce/onboot.lst || echo 'sonos-pi3-plugins.tcz' >> /mnt/mmcblk0p2/tce/onboot.lst
pcp bu
echo "Added sonos-pi3-plugins.tcz to /mnt/mmcblk0p2/tce/onboot.lst"
