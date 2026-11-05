#!/bin/sh

echo "=== ASGN1 DRIVER TEST SCRIPT ==="

echo "[1] Loading driver..."
insmod /lib/modules/5.15.0/asgn1_driver.ko

if [ $? -ne 0 ]; then
    echo "Driver loading failed!"
    exit 1
fi

echo "[2] Creating device node..."
mknod /dev/asgn1 c 241 0
chmod 666 /dev/asgn1

echo "[3] Running ioctl test..."
/bin/test_asgn1

echo "[4] Checking empty buffer..."
cat /dev/asgn1

echo "[5] Removing device node..."
rm -f /dev/asgn1

echo "[6] Unloading driver..."
rmmod asgn1_driver

echo "[7] Kernel log..."
dmesg | tail -n 30

echo "=== TEST COMPLETED ==="

