# Load the required .NET graphics assembly
Add-Type -AssemblyName System.Drawing

$OutputFile = Join-Path (get-Location) "src/kernel/kboot/kdefs.h"
$FontName = "Consolas"
$FontSize = 9 # Adjust to fit the 8x8 grid

# Define the C header and macro templates
$C_Header = @"
#pragma once

#include "kboot.h"

#define DEFINE_GLYPH(NAME, MATCHER, W, H, V_STRIDE, H_STRIDE, ...) \
    const FontGlyph NAME = { \
        .Matcher = MATCHER, \
        .BitWidth = W, \
        .BitHeight = H, \
        .VerticalStride = V_STRIDE, \
        .HorizontalStride = H_STRIDE, \
        .Data = { __VA_ARGS__ } \
    }

"@

$C_Body = @()
$C_Pointers = @("const FontGlyph* ASCII[] = {")

# Setup the drawing environment
$font = New-Object System.Drawing.Font($FontName, $FontSize, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
$brush = [System.Drawing.Brushes]::White
$format = New-Object System.Drawing.StringFormat
$format.Alignment = [System.Drawing.StringAlignment]::Center
$format.LineAlignment = [System.Drawing.StringAlignment]::Center
$rect = New-Object System.Drawing.RectangleF(0, 0, 8, 8)

# Iterate through standard printable ASCII (32 Space to 126 Tilde)
for ($ascii = 32; $ascii -le 126; $ascii++) {
    $char = [char]$ascii
    
    # Handle characters that require special naming in C
    $varName = "glyph_$ascii"
    if ($char -match '[a-zA-Z0-9]') { $varName = "glyph_$char" }
    elseif ($ascii -eq 32) { $varName = "glyph_space" }
    elseif ($ascii -eq 92) { $varName = "glyph_backslash" }
    
    # Create an 8x8 bitmap and draw the character
    $bmp = New-Object System.Drawing.Bitmap(8, 8)
    $graphics = [System.Drawing.Graphics]::FromImage($bmp)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit
    $graphics.Clear([System.Drawing.Color]::Black)
    $graphics.DrawString($char.ToString(), $font, $brush, $rect, $format)
    
    # Extract the pixels into a hex byte array (1 bit per pixel)
    $hexRowStrings = @()
    for ($y = 0; $y -lt 8; $y++) {
        $rowByte = 0
        for ($x = 0; $x -lt 8; $x++) {
            $pixel = $bmp.GetPixel($x, $y)
            if ($pixel.R -gt 127) { # If pixel is white
                $rowByte = $rowByte -bor (1 -shl (7 - $x))
            }
        }
        $hexRowStrings += "0x$($rowByte.ToString('X2'))"
    }
    
    # Format the C macro call
    $hexData = $hexRowStrings -join ", "
    
    # Escape single quotes and backslashes for the Matcher character literal
    $cChar = $char
    if ($char -eq "'") { $cChar = "\'" }
    if ($char -eq "\") { $cChar = "\\" }
    
    $C_Body += "DEFINE_GLYPH($varName, '$cChar', 8, 8, 0, 0, $hexData);"
    $C_Pointers += "    &$varName,"
    
    $graphics.Dispose()
    $bmp.Dispose()
}

$C_Pointers += @"
};

uint32_t ASCIILength = sizeof(ASCII) / sizeof(FontGlyph *);
"@

# Write everything to the output file
$FinalOutput = $C_Header + "`n" + ($C_Body -join "`n`n") + "`n`n" + ($C_Pointers -join "`n")
Set-Content -Path $OutputFile -Value $FinalOutput

Write-Host "Generated C font file at: $OutputFile" -ForegroundColor Green