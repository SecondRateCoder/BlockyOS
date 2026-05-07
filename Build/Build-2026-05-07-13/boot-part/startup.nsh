@echo -off

cls
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
endif

:found
echo "Found BOOTX64.efi on %fs%"

echo "Do you want to add a \"BlockyOS Boot Image\" for MyApp?"
echo "Press any other key to add, or "Q/q" to skip."
pause

echo Adding boot entry...
bcfg boot add 0 "%fs%\\EFI\\BOOT\\BOOTX64.efi" "BlockyOS Boot Image"
echo "Boot entry added successfully."
echo "Press any key to finish..."
pause