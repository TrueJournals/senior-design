#!/bin/bash

# This script is the update script which will be run on the Pi after the update
# package is extracted.  This script should compile all the items, and copy the
# files into their correct locations.  Additionally, this script can communicate
# with the other Pi to copy files to it over SFTP.

echo "Running update script"

# "Update light" -- Set GPIO 4 to output and turn on
echo "4" | sudo tee /sys/class/gpio/export
echo "out" | sudo tee /sys/class/gpio/gpio4/direction
echo "1" | sudo tee /sys/class/gpio/gpio4/value

# Build all necessary files
echo "Building and copying library"
cd ./libs/
bash ./build-lib.sh >> /tmp/update-log 2>&1
# Copy these files first, as they're necessary for later builds
sudo cp ./libptp++.so /usr/lib/
sudo cp ./libptp++.hpp /usr/include/
sudo cp ./live_view.h /usr/include/

echo "Building submarine executable"
cd ../sd-submarine/
bash ./build-submarine.sh >> /tmp/update-log 2>&1

echo "Building surface executable"
cd ../sd-surface/
bash ./build-surface.sh >> /tmp/update-log 2>&1
cd ../

# Copy files to correct locations
echo "Copying files locally"
# Submarine file
sudo cp ./sd-submarine/sd-submarine /usr/bin/
# Surface file
sudo cp ./sd-surface/sd-surface /usr/bin/
# util files
sudo cp ./utils/90-senior-design.rules /etc/udev/rules.d/
sudo cp ./utils/sd-start /usr/sbin/
sudo cp ./utils/sd-stop /usr/sbin/
sudo cp ./utils/sd-update /usr/sbin/
sudo cp ./utils/sd-startup /etc/init.d/

# Update init.d files
echo "Reloading init.d rules"
sudo update-rc.d sd-startup defaults

# Restart udev to pickup new rules
echo "Reloading udev"
#sudo /etc/init.d/udev restart
sudo udevadm control --reload-rules

# Restart the UI
echo "Restarting UI"
sudo /etc/init.d/sd-startup stop
sudo /etc/init.d/sd-startup start

MY_HOSTNAME=`hostname -s`
if [ "${MY_HOSTNAME}" = "pi-surface" ]; then
    OTHER_HOSTNAME="pi-submarine"
else
    OTHER_HOSTNAME="pi-surface"
fi
echo "Detected that I am: ${MY_HOSTNAME}, other Pi is: ${OTHER_HOSTNAME}"

echo "SSHing to make directories"
ssh pi@$OTHER_HOSTNAME << EOF
mkdir -p /tmp/update/usr/lib/
mkdir -p /tmp/update/usr/include/
mkdir -p /tmp/update/usr/bin/
mkdir -p /tmp/update/etc/udev/rules.d/
mkdir -p /tmp/update/usr/sbin/
mkdir -p /tmp/update/etc/init.d/
EOF

# TODO: Set up public key authentication so that we can SFTP/SSH
# without providing credentials
# TODO: I'll need to SFTP into root to have permissions to copy into the right
#  locations, won't I?
#  Alternatively, let's copy these to /tmp, then run a script to move them
#  to the right place :/
echo "Copying files"
sftp pi@$OTHER_HOSTNAME << EOF
put /usr/lib/libptp++.so /tmp/update/usr/lib/
put /usr/include/libptp++.hpp /tmp/update/usr/include/
put /usr/bin/sd-submarine /tmp/update/usr/bin/
put /usr/bin/sd-surface /tmp/update/usr/bin/
put /etc/udev/rules.d/90-senior-design.rules /tmp/update/etc/udev/rules.d/
put /usr/sbin/sd-start /tmp/update/usr/sbin/
put /usr/sbin/sd-stop /tmp/update/usr/sbin/
put /usr/sbin/sd-update /tmp/update/usr/sbin/
put /etc/init.d/sd-startup /tmp/update/etc/init.d/
EOF

# User pi will need sudo access with NOPASSWD, which is a security risk.
# We're OK with this because neither pi is connected to the Internet, and anyone
# with access has physical access anyway.

# This copy line might not be ideal, but it's fine as long as we set things up
# correctly above.  We can always reimage the SD card if something goes wrong.
echo "SSH to move files and restart UI"
ssh pi@$OTHER_HOSTNAME << EOF
sudo cp -r /tmp/update/* /
sudo /etc/init.d/sd-startup stop
sudo /etc/init.d/sd-startup start
EOF

# "Update light" -- turn off and clean up
echo "0" | sudo tee /sys/class/gpio/gpio4/value
echo "4" | sudo tee /sys/class/gpio/unexport

exit 0
