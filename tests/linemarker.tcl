source base.tcl

set w .line
set graph [bltLineGraph $w]

set mm [$graph marker create line tt -element data1 \
	    -coords {1 50 1.5 100 1 150} -linewidth 5]
set nn [$graph marker create line ss -element data1 \
	    -coords {1 150 .5 100 1 50} -linewidth 1 \
	    -outline green -dashes 4]

#bltTest3 $graph marker $mm -bindtags
bltTest3 $graph marker $mm -cap round
bltTest3 $graph marker $mm -coords {1 50 1.5 100 2 150}
bltTest3 $graph marker $mm -dashes dashdot
bltTest3 $graph marker $mm -dashoffset 10
bltTest3 $graph marker $mm -element data2
bltTest3 $graph marker $nn -fill yellow
bltTest3 $graph marker $mm -join round
bltTest3 $graph marker $mm -linewidth 1
bltTest3 $graph marker $mm -hide yes
bltTest3 $graph marker $mm -mapx x
bltTest3 $graph marker $mm -mapy y
bltTest3 $graph marker $mm -outline green
bltTest3 $graph marker $mm -state disabled
bltTest3 $graph marker $mm -under yes
bltTest3 $graph marker $mm -xoffset 20
bltTest3 $graph marker $mm -xor yes
bltTest3 $graph marker $mm -yoffset 20

#bltCmd $graph marker bind marker ?sequence command?
bltCmd $graph marker cget $mm -cap
bltCmd $graph marker configure $mm
bltCmd $graph marker configure $mm -cap
set foo [$graph marker create line]
bltCmd $graph marker delete $foo
set foo [$graph marker create line foo]
bltCmd $graph marker delete $foo
bltCmd $graph marker exists $mm
bltCmd $graph marker find enclosed 0 0 2 200
bltCmd $graph marker get name current
bltCmd $graph marker lower $mm
bltCmd $graph marker lower $mm $nn
bltCmd $graph marker names
bltCmd $graph marker raise $mm
bltCmd $graph marker raise $mm $nn
bltCmd $graph marker type $mm

echo "done"
bltPlotDestroy $w

