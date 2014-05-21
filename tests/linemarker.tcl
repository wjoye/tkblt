source base.tcl

set w .line
set graph [bltLineGraph $w]

set mm [$graph marker create line tt -element data2 \
	    -coords {1 50 1.5 100 1 150} -linewidth 5]
set nn [$graph marker create line ss -element data2 \
	    -coords {1 150 .5 100 1 50} -linewidth 1 \
	    -outline green -dashes 4]
$graph element configure data1 -hide yes

echo "Testing Line Marker..."
bltCmd $graph marker configure $mm

#bltTest3 $graph marker $mm -bindtags {aa}
bltTest3 $graph marker $mm -cap round
bltTest3 $graph marker $mm -coords {1 50 1.5 100 2 150}
bltTest3 $graph marker $mm -dashes {8 3}
bltTest3 $graph marker $mm -dashoffset 10
bltTest3 $graph marker $mm -element data1
bltTest3 $graph marker $nn -fill yellow
bltTest3 $graph marker $mm -join round
bltTest3 $graph marker $mm -linewidth 1
bltTest3 $graph marker $mm -hide yes
bltTest3 $graph marker $mm -mapx x2
bltTest3 $graph marker $mm -mapy y2
bltTest3 $graph marker $mm -outline green
bltTest3 $graph marker $mm -state disabled
bltTest3 $graph marker $mm -under yes
bltTest3 $graph marker $mm -xoffset 20
bltTest3 $graph marker $mm -xor yes
bltTest3 $graph marker $mm -yoffset 20

echo "done"
bltPlotDestroy $w

