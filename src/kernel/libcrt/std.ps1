$Path = 'src\kernel\libcrt\std.asm'

for($cc = 0; $cc -lt 256; ++$cc){Add-Content -Path $Path -Value "`tint $cc"}