source base.tcl

set w .line
set graph [bltLineGraph $w]

set mm [$graph marker create bitmap tt -element data2 \
	    -coords {1.5 100} -bitmap error]
$graph element configure data1 -hide yes

echo "Testing Bitmap Marker..."
bltCmd $graph marker configure $mm

bltTest3 $graph marker $mm -anchor nw $dops
bltTest3 $graph marker $mm -background yellow $dops
bltTest3 $graph marker $mm -bg red $dops
bltTest3 $graph marker $mm -bindtags {aa} 0
bltTest3 $graph marker $mm -bitmap hourglass $dops
bltTest3 $graph marker $mm -coords {1 50} $dops
bltTest3 $graph marker $mm -element data1 $dops
bltTest3 $graph marker $mm -fg cyan $dops
bltTest3 $graph marker $mm -fill yellow $dops
bltTest3 $graph marker $mm -foreground blue $dops
bltTest3 $graph marker $mm -hide yes $dops
bltTest3 $graph marker $mm -mapx x2 $dops
bltTest3 $graph marker $mm -mapy y2 $dops
bltTest3 $graph marker $mm -outline green $dops
bltTest3 $graph marker $mm -state disabled $dops
bltTest3 $graph marker $mm -under yes $dops
bltTest3 $graph marker $mm -xoffset 20 $dops
bltTest3 $graph marker $mm -yoffset 20 $dops

echo "done"
bltPlotDestroy $w

