source base.tcl

set w .line
set graph [bltLineGraph $w]
return
set marker [$graph marker create line tt \
		-element data1 \
		-coords "1 50 1.5 100 1 150" \
		-linewidth 10]

#bltTest2 $graph $marker -bindtags
bltTest2 $graph $marker -cap round
#bltTest2 $graph $marker -coords
bltTest2 $graph $marker -dashes dashdot
bltTest2 $graph $marker -dashes dashdot dashoffset 10
bltTest2 $graph $marker -element data2
#bltTest2 $graph $marker -fill
bltTest2 $graph $marker -join round
bltTest2 $graph $marker -linewidth 1
bltTest2 $graph $marker -hide yes
#bltTest2 $graph $marker -mapx
#bltTest2 $graph $marker  -mapy
bltTest2 $graph $marker -outline green
#bltTest2 $graph $marker -state
#bltTest2 $graph $marker -under
#bltTest2 $graph $marker -xoffset
#bltTest2 $graph $marker -xor
#bltTest2 $graph $marker -yoffset

#bltCmd $graph marker axis
#bltCmd $graph marker bind marker ?sequence command?
#bltCmd $graph marker cget marker ?option?
#bltCmd $graph marker configure marker ?option value?...
#bltCmd $graph marker create type marker ?option value?...
bltCmd $graph marker exists $marker
#bltCmd $graph marker find "enclosed|overlapping x1 y1 x2 y2"
#bltCmd $graph marker get name
#bltCmd $graph marker lower "marker ?afterMarker?"
#bltCmd $graph marker names "?pattern?..."
#bltCmd $graph marker raise "marker ?beforeMarker?"
bltCmd $graph marker type $marker
bltCmd $graph marker delete $marker

echo "done"
#bltPlotDestroy $w

