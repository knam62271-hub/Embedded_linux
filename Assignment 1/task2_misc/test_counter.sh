#!/bin/sh

echo "=== COUNTER DRIVER TEST ==="

insmod /lib/modules/5.15.0/counter_driver.ko

DEV=$(cat /sys/class/misc/counter/dev)
MAJOR=${DEV%:*}
MINOR=${DEV#*:}

mknod /dev/counter c $MAJOR $MINOR
chmod 666 /dev/counter

echo "Initial counter:"
cat /proc/counter_info

echo "Read test:"
cat /dev/counter
cat /dev/counter
cat /dev/counter

echo "Write test:"
echo 100 > /dev/counter
cat /proc/counter_info

echo "Reset test:"
echo 0 > /dev/counter
cat /proc/counter_info

echo "Invalid input test:"
echo -5 > /dev/counter
echo abc > /dev/counter

echo "Final information:"
cat /proc/counter_info

rm /dev/counter
rmmod counter_driver

echo "=== TEST COMPLETED ==="
