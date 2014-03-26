source base.tcl

set w .line
set graph [bltLineGraph $w]

set mm [$graph marker create text tt -element data1 \
	    -coords {1.5 100} -text {Text Marker} -font {24}]

echo "Testing Text Marker..."

bltTest3 $graph marker $mm -anchor nw
bltTest3 $graph marker $mm -background blue
bltTest3 $graph marker $mm -bg red
bltTest3 $graph marker $mm -bindtags {aa}
bltTest3 $graph marker $mm -coords {1 50}
bltTest3 $graph marker $mm -element data2
bltTest3 $graph marker $mm -fg cyan
bltTest3 $graph marker $mm -fill yellow
bltTest3 $graph marker $mm -font {times 24 bold italic}
bltTest3 $graph marker $mm -foreground red
bltTest3 $graph marker $mm -justify right
bltTest3 $graph marker $mm -hide yes
bltTest3 $graph marker $mm -mapx x
bltTest3 $graph marker $mm -mapy y
bltTest3 $graph marker $mm -outline green
bltTest3 $graph marker $mm -rotate 45
bltTest3 $graph marker $mm -state disabled
bltTest3 $graph marker $mm -text {Hello World}
bltTest3 $graph marker $mm -under yes
bltTest3 $graph marker $mm -xoffset 20
bltTest3 $graph marker $mm -yoffset 20

echo "done"
bltPlotDestroy $w

