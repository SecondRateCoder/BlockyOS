@echo -off
echo "=== Auto-Detecting Filesystem ==="
map -r

set found 0

for %d in fs0 fs1 fs2 fs3 fs4 fs5
  if exist %d:\\EFI\\BOOT\\BOOTX64.efi then
    set fs %d:
    set found 1
    goto found
  endif
endfor

if %found% == 0 then
  echo "ERROR: Could not find BOOTX64.efi"
  echo "Press any key to exit..."
  pause
  exit
endif

:found
echo "Found BOOTX64.efi on %fs%"

echo "Do you want to add a UEFI Boot Entry for MyApp?"
echo "Press Y to add, or any other key to skip."
echo "Press a key..."
pause

# Read last key pressed
set key %lasterror%

# lasterror returns:
# 0x15 = 'Y' key
# 0x31 = 'y' key

if %key% == 0x15 then
	goto add
endif
if %key% == 0x31 then
	goto add
endif

echo "Skipping boot entry creation."
echo "Press any key to finish..."
pause
exit

:add
echo Adding boot entry...
bcfg boot add 0 "%fs%\\EFI\\MyApp\MyApp.efi" "BlockyOS Boot Image"
echo "Boot entry added successfully."
echo "Press any key to finish..."
pause