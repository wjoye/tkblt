set sleep 1000

proc bltPlot {w title} {
    toplevel $w
    wm title $w $title
    wm protocol $w WM_DELETE_WINDOW [list bltPlotDestroy $w]

    set mb ${w}mb
    menu $mb
    $w configure -menu $mb
}

proc bltPlotDestroy {w} {
    destroy ${w}mb
    destroy $w
}

proc bltTest {graph option value} {
    global sleep

    echo "  $option $value"
    set org [$graph cget $option]
    $graph configure $option $value
    update
#    read stdin
    after $sleep
    $graph configure $option $org
    update
    after $sleep
}

proc bltTest2 {graph which option value} {
    global sleep

    echo "  $option $value"
    set org [$graph $which cget $option]
    $graph $which configure $option $value
    update
#    read stdin
    after $sleep
    $graph $which configure $option $org
    update
    after $sleep
}

proc bltTest3 {graph which item option value} {
    global sleep

    echo "  $item $option $value"
    set org [$graph $which cget $item $option]
    $graph $which configure $item $option $value
    update
    after $sleep
    $graph $which configure $item $option $org
    update
    after $sleep
}

proc bltCmd {graph args} {
    global sleep

    echo " $graph $args"
    eval $graph $args
    update
    after $sleep
}

proc bltElements {graph} {
    $graph element create data1 \
	-xdata { 0.2 0.4 0.6 0.8 1.0 1.2 1.4 1.6 1.8 2.0 } \
	-ydata { 13 25 36 46 55 64 70 75 80 90}
    $graph element create data2 \
	-xdata { 0.2 0.4 0.6 0.8 1.0 1.2 1.4 1.6 1.8 2.0 } \
	-ydata { 26 50 72 92 110 128 140 150 160 180} \
 	-yerror {10 10 10 10 10 10 10 10 10 10 10}  \
	-color red
    $graph legend configure -title "Legend"
}

proc bltBarGraph {w} {
    global sleep

    set title "Bar Graph"
    bltPlot $w $title
    set graph [blt::barchart ${w}.gr \
		   -width 600 \
		   -height 500 \
		   -title $title \
		   -barwidth .2 \
		   -barmode aligned \
		  ]
    pack $graph -expand yes -fill both
    bltElements $graph

    update
    after $sleep
    return $graph
}

proc bltLineGraph {w} {
    global sleep

    set title "Line Graph"
    bltPlot $w $title
    set graph [blt::graph ${w}.gr \
		   -width 600 \
		   -height 500 \
		   -title $title \
		  ]
    pack $graph -expand yes -fill both
    bltElements $graph

    update
    after $sleep
    return $graph
}
