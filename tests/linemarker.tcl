source base.tcl

set w .line
set graph [bltLineGraph $w]

set mm [$graph marker create line tt -element data1 \
	    -coords "1 50 1.5 100 1 150" -linewidth 10]
return
#bltTest3 $graph marker $mm -bindtags
bltTest3 $graph marker $mm -cap round
#bltTest3 $graph marker $mm -coords
bltTest3 $graph marker $mm -dashes dashdot
bltTest3 $graph marker $mm -dashoffset 10
bltTest3 $graph marker $mm -element data2
#bltTest3 $graph marker $mm -fill
bltTest3 $graph marker $mm -join round
bltTest3 $graph marker $mm -linewidth 1
bltTest3 $graph marker $mm -hide yes
bltTest3 $graph marker $mm -mapx x
bltTest3 $graph marker $mm  -mapy y
bltTest3 $graph marker $mm -outline green
#bltTest3 $graph marker $mm -state
bltTest3 $graph marker $mm -under yes
bltTest3 $graph marker $mm -xoffset 20
bltTest3 $graph marker $mm -xor yes
bltTest3 $graph marker $mm -yoffset 20

#bltCmd $graph marker bind marker ?sequence command?
#bltCmd $graph marker cget 
#bltCmd $graph marker configure 
set foo [$graph marker create line]
bltCmd $graph marker delete $foo
set foo [$graph marker create line foo]
bltCmd $graph marker delete $foo
bltCmd $graph marker exists $mm
#bltCmd $graph marker find "enclosed|overlapping x1 y1 x2 y2"
#bltCmd $graph marker get name
bltCmd $graph marker lower $mm
bltCmd $graph marker names
bltCmd $graph marker raise $mm
bltCmd $graph marker type $mm

echo "done"
#bltPlotDestroy $w

