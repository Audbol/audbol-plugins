<#
Generates the Audbol brand artwork into resources/brand/:
  audbol.ico               app / plug-in folder icon (16..256 px)
  wizard-*.bmp             installer side panel (100 % and 200 %)
  wizard-small-*.bmp       installer header mark (100 % and 200 %)
  audbol-logo.png          512 px logo for README / repository

The look matches the plug-ins: graphite background, the knob mark and
lowercase wordmark from the editor header (Bahnschrift), the suite accent
#6CAAFF, an EQ-curve motif and the eleven plug-in accent colours.

  & .\tools\release\brand_assets.ps1
#>
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
$out = "$root\resources\brand"
New-Item -ItemType Directory -Force $out | Out-Null

$bg = [Drawing.Color]::FromArgb(255, 15, 16, 19)
$panel = [Drawing.Color]::FromArgb(255, 23, 25, 29)
$accent = [Drawing.Color]::FromArgb(255, 108, 170, 255)
$text = [Drawing.Color]::FromArgb(255, 234, 236, 240)
$dim = [Drawing.Color]::FromArgb(255, 146, 151, 162)
# The plug-ins' own accents (suite::ui::accents), in installer order
$pluginAccents = @((62, 207, 190), (108, 170, 255), (240, 122, 170), (255, 112, 92), (162, 142, 250), (86, 200, 240),
                   (255, 150, 72), (122, 218, 150), (172, 186, 206), (192, 228, 92), (250, 208, 96)) |
    ForEach-Object { [Drawing.Color]::FromArgb(255, $_[0], $_[1], $_[2]) }
function Brand-Font([double]$px, [bool]$bold) {
    New-Object Drawing.Font ($(if ($bold) { "Bahnschrift SemiBold" } else { "Bahnschrift" })), $px, ([Drawing.GraphicsUnit]::Pixel)
}

function New-Canvas([int]$w, [int]$h) {
    $bmp = New-Object Drawing.Bitmap $w, $h, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = "AntiAlias"
    $g.InterpolationMode = "HighQualityBicubic"
    $g.PixelOffsetMode = "HighQuality"
    $g.TextRenderingHint = "AntiAliasGridFit"
    return @($bmp, $g)
}

# The Audbol mark, as in the editor header: a knob, i.e. a ring with a
# pointer toward ten o'clock. Small sizes get a heavier stroke.
function Draw-Mark($g, [double]$cx, [double]$cy, [double]$r, [bool]$detail) {
    $w = $r * ($(if ($detail) { 0.27 } else { 0.36 }))
    $pen = New-Object Drawing.Pen $accent, $w
    $pen.StartCap = "Round"; $pen.EndCap = "Round"
    $rr = $r - $w / 2
    $g.DrawEllipse($pen, ($cx - $rr), ($cy - $rr), (2 * $rr), (2 * $rr))
    $d = 0.70710678
    $g.DrawLine($pen, ($cx - $r * 0.2 * $d), ($cy - $r * 0.2 * $d), ($cx - $r * 0.55 * $d), ($cy - $r * 0.55 * $d))
}

# A row of dots in the plug-ins' accent colours, centred on $cx.
function Draw-Accents($g, [double]$cx, [double]$y, [double]$dot, [double]$gap) {
    $x = $cx - ($pluginAccents.Count * ($dot + $gap) - $gap) / 2
    foreach ($c in $pluginAccents) {
        $g.FillEllipse((New-Object Drawing.SolidBrush $c), $x, $y, $dot, $dot)
        $x += $dot + $gap
    }
}

function Rounded-Rect([double]$x, [double]$y, [double]$w, [double]$h, [double]$r) {
    $p = New-Object Drawing.Drawing2D.GraphicsPath
    $d = 2 * $r
    $p.AddArc($x, $y, $d, $d, 180, 90); $p.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $p.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90); $p.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $p.CloseFigure(); return $p
}

# Icon image: dark rounded tile with the mark. Small sizes drop the detail.
function Icon-Bitmap([int]$s) {
    $c, $g = New-Canvas $s $s
    $g.Clear([Drawing.Color]::Transparent)
    $pad = [Math]::Max(0.5, $s * 0.03)
    $g.FillPath((New-Object Drawing.SolidBrush $bg), (Rounded-Rect $pad $pad ($s - 2 * $pad) ($s - 2 * $pad) ($s * 0.2)))
    Draw-Mark $g ($s / 2) ($s / 2) ($s * 0.34) ($s -ge 32)
    $g.Dispose()
    return $c
}

# ICO entry: PNG for 256 px, classic 32-bit DIB (BGRA + AND mask) below,
# which every Windows component and host can read.
function Icon-Entry([int]$s) {
    $bmp = Icon-Bitmap $s
    $ms = New-Object IO.MemoryStream
    if ($s -ge 256) {
        $bmp.Save($ms, [Drawing.Imaging.ImageFormat]::Png)
    } else {
        $bw = New-Object IO.BinaryWriter $ms
        $bw.Write([uint32]40); $bw.Write([int32]$s); $bw.Write([int32](2 * $s))   # height covers XOR + AND
        $bw.Write([uint16]1); $bw.Write([uint16]32); $bw.Write([uint32]0)
        $bw.Write([uint32]0); $bw.Write([int32]0); $bw.Write([int32]0); $bw.Write([uint32]0); $bw.Write([uint32]0)
        for ($y = $s - 1; $y -ge 0; $y--) {       # bottom-up rows
            for ($x = 0; $x -lt $s; $x++) {
                $p = $bmp.GetPixel($x, $y)
                $bw.Write([byte]$p.B); $bw.Write([byte]$p.G); $bw.Write([byte]$p.R); $bw.Write([byte]$p.A)
            }
        }
        $maskRow = [int]([Math]::Ceiling($s / 32.0) * 4)                       # 1-bit rows, 4-byte aligned
        $bw.Write((New-Object byte[] ($maskRow * $s)))                          # alpha channel is used instead
    }
    $bmp.Dispose()
    return ,([byte[]]$ms.ToArray())
}

# --- audbol.ico ----------------------------------------------------------------
$sizes = 16, 20, 24, 32, 40, 48, 64, 128, 256
$images = New-Object 'System.Collections.Generic.List[byte[]]'
foreach ($s in $sizes) { $images.Add((Icon-Entry $s)) }
$ico = New-Object IO.MemoryStream
$w = New-Object IO.BinaryWriter $ico
$w.Write([uint16]0); $w.Write([uint16]1); $w.Write([uint16]$sizes.Count)
$offset = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $s = $sizes[$i]; $len = $images[$i].Length
    $dim8 = if ($s -ge 256) { 0 } else { $s }
    $w.Write([byte]$dim8); $w.Write([byte]$dim8)
    $w.Write([byte]0); $w.Write([byte]0); $w.Write([uint16]1); $w.Write([uint16]32)
    $w.Write([uint32]$len); $w.Write([uint32]$offset); $offset += $len
}
foreach ($img in $images) { $w.Write($img, 0, $img.Length) }
$w.Flush()
[IO.File]::WriteAllBytes("$out\audbol.ico", $ico.ToArray())

# --- Installer side panel ----------------------------------------------------
function Wizard-Image([int]$scale) {
    $W = 164 * $scale; $H = 314 * $scale
    $c, $g = New-Canvas $W $H
    $grad = New-Object Drawing.Drawing2D.LinearGradientBrush (New-Object Drawing.Point 0, 0), (New-Object Drawing.Point 0, $H), $panel, $bg
    $g.FillRectangle($grad, 0, 0, $W, $H)
    # Faint grid, like the plug-in displays
    $gridPen = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(255, 36, 40, 48)), (1 * $scale)
    for ($x = 0; $x -le $W; $x += 20 * $scale) { $g.DrawLine($gridPen, $x, 150 * $scale, $x, $H) }
    for ($y = 150 * $scale; $y -le $H; $y += 20 * $scale) { $g.DrawLine($gridPen, 0, $y, $W, $y) }
    # EQ-curve motif with a soft fill below
    $pts = for ($i = 0; $i -le 60; $i++) {
        $x = $W * $i / 60
        $t = $i / 60
        $yv = 236 - 38 * [Math]::Exp(-[Math]::Pow(($t - 0.32) / 0.11, 2)) + 22 * [Math]::Exp(-[Math]::Pow(($t - 0.72) / 0.08, 2)) - 10 * $t
        New-Object Drawing.PointF $x, ($yv * $scale)
    }
    $fill = New-Object Drawing.Drawing2D.GraphicsPath
    $fill.AddLines($pts); $fill.AddLine($W, $H, 0, $H); $fill.CloseFigure()
    $g.FillPath((New-Object Drawing.SolidBrush ([Drawing.Color]::FromArgb(40, $accent.R, $accent.G, $accent.B))), $fill)
    $g.DrawLines((New-Object Drawing.Pen $accent, (2.2 * $scale)), $pts)
    # Mark + wordmark
    Draw-Mark $g ($W / 2) (52 * $scale) (20 * $scale) $true
    $sf = New-Object Drawing.StringFormat; $sf.Alignment = "Center"
    $g.DrawString("audbol", (Brand-Font (24 * $scale) $true),
        (New-Object Drawing.SolidBrush $text), (New-Object Drawing.RectangleF 0, (82 * $scale), $W, (32 * $scale)), $sf)
    $g.DrawString("Plug-in Suite", (Brand-Font (10 * $scale) $false),
        (New-Object Drawing.SolidBrush $dim), (New-Object Drawing.RectangleF 0, (113 * $scale), $W, (16 * $scale)), $sf)
    Draw-Accents $g ($W / 2) (294 * $scale) (5 * $scale) (4 * $scale)
    $g.Dispose()
    # Inno wants 24-bit BMP
    $flat = $c.Clone((New-Object Drawing.Rectangle 0, 0, $W, $H), [Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $flat.Save("$out\wizard-$($W)x$($H).bmp", [Drawing.Imaging.ImageFormat]::Bmp)
    $flat.Dispose(); $c.Dispose()
}
Wizard-Image 1
Wizard-Image 2

# --- Installer header mark (on the wizard's white header) -------------------
foreach ($s in 55, 110) {
    $c, $g = New-Canvas $s $s
    $g.Clear([Drawing.Color]::White)
    $g.FillPath((New-Object Drawing.SolidBrush $bg), (Rounded-Rect ($s * 0.06) ($s * 0.06) ($s * 0.88) ($s * 0.88) ($s * 0.18)))
    Draw-Mark $g ($s / 2) ($s / 2) ($s * 0.3) $true
    $g.Dispose()
    $flat = $c.Clone((New-Object Drawing.Rectangle 0, 0, $s, $s), [Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $flat.Save("$out\wizard-small-$s.bmp", [Drawing.Imaging.ImageFormat]::Bmp)
    $flat.Dispose(); $c.Dispose()
}

# --- Logo for README / repository ------------------------------------------
$c, $g = New-Canvas 1200 360
$g.Clear($bg)
Draw-Mark $g 180 172 88 $true
$g.DrawString("audbol", (Brand-Font 168 $true), (New-Object Drawing.SolidBrush $text), 300, 52)
$g.DrawString("Plug-in Suite", (Brand-Font 40 $false), (New-Object Drawing.SolidBrush $dim), 314, 244)
Draw-Accents $g 905 266 14 10
$g.Dispose()
$c.Save("$out\audbol-logo.png", [Drawing.Imaging.ImageFormat]::Png); $c.Dispose()

Get-ChildItem $out | ForEach-Object { "{0,-26} {1,8:N0} bytes" -f $_.Name, $_.Length }
