source base.tcl

set w .line
set graph [bltLineGraph $w]

echo "Testing Crosshairs..."

$graph crosshairs configure -hide no
$graph crosshairs configure -position "@200,200"

bltTest2 $graph crosshairs -color green
bltTest2 $graph crosshairs -dashes "dashdot"
bltTest2 $graph crosshairs -dashes "5 8 3"
bltTest2 $graph crosshairs -hide yes
bltTest2 $graph crosshairs -linewidth 3
bltTest2 $graph crosshairs -position "@100,100"

bltCmd $graph crosshairs cget -color
bltCmd $graph crosshairs configure
bltCmd $graph crosshairs configure -color
bltCmd $graph crosshairs on
bltCmd $graph crosshairs off
bltCmd $graph crosshairs toggle

echo "done"
bltPlotDestroy $w

