set all 0
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
    after $sleep
    $graph $which configure $option $org
    update
    after $sleep
}

set w .bar
set title "Bar Graph Test"
bltPlot $w $title
set graph [blt::barchart ${w}.gr \
	       -width 600 \
	       -height 500 \
	       -title $title \
	       -barwidth .2 \
	       -barmode aligned \
	      ]
pack $graph -expand yes -fill both

$graph element create bar1 \
    -xdata { 0.2 0.4 0.6 0.8 1.0 1.2 1.4 1.6 1.8 2.0 } \
    -ydata { 13 25 36 46 55 64 70 75 80 90}\
    -color blue
$graph element create bar2 \
    -xdata { 0.2 0.4 0.6 0.8 1.0 1.2 1.4 1.6 1.8 2.0 } \
    -ydata { 26 50 72 92 110 128 140 150 160 180}\
    -color red
$graph legend configure -title "Legend"

update
after $sleep

# Graph
if {$all} {
    bltTest $graph -aspect 2
    bltTest $graph -background red
    bltTest $graph -barmode overlap
    bltTest $graph -barwidth .15
    #bltTest $graph -baseline
    bltTest $graph -bd 50
    bltTest $graph -bg green
    bltTest $graph -bm 50
    bltTest $graph -borderwidth 50
    bltTest $graph -bottommargin 50
    #bltTest $graph -bottomvariable
    #bltTest $graph -bufferelements
    #bltTest $graph -buffergraph
    bltTest $graph -cursor cross
    #bltTest $graph -fg
    bltTest $graph -font "times 36 bold italic"
    #bltTest $graph -foreground
    #bltTest $graph -halo
    bltTest $graph -height 300
    #bltTest $graph -highlightbackground
    #bltTest $graph -highlightcolor
    #bltTest $graph -highlightthickness
    bltTest $graph -invertxy yes
    #bltTest $graph -justify
    bltTest $graph -leftmargin 50
    #bltTest $graph -leftvariable
    bltTest $graph -lm 50
    bltTest $graph -plotbackground cyan
    bltTest $graph -plotborderwidth 50
    bltTest $graph -plotpadx 50
    bltTest $graph -plotpady 50
    bltTest $graph -plotrelief groove
    bltTest $graph -relief groove
    bltTest $graph -rightmargin 50
    #bltTest $graph -rightvariable
    bltTest $graph -rm 50
    #bltTest $graph -stackaxes
    #bltTest $graph -takefocus
    bltTest $graph -title "This is a Title"
    bltTest $graph -tm 50
    bltTest $graph -topmargin 50
    #bltTest $graph -topvariable
    bltTest $graph -width 300
    bltTest $graph -plotwidth 300
    bltTest $graph -plotheight 300
}

# Grid

# Crosshairs

if {$all} {
    #bltTest2 $graph crosshairs -color green
    #bltTest2 $graph crosshairs -dashes "dotdot"
    #bltTest2 $graph crosshairs -dashes "5 8 3"
    #bltTest2 $graph crosshairs -hide yes
    #bltTest2 $graph crosshairs -linewidth 3
    #bltTest2 $graph crosshairs -position "@1,50"

    #$graph crosshairs on
    #$graph crosshairs off
    #$graph crosshairs toggle
}

# Legend

if {$all} {
    #bltTest2 $graph legend -activebackground
    #bltTest2 $graph legend -activeborderwidth
    #bltTest2 $graph legend -activeforeground
    #bltTest2 $graph legend -activerelief
    bltTest2 $graph legend -anchor s
    bltTest2 $graph legend -bg pink
    bltTest2 $graph legend -background cyan
    bltTest2 $graph legend -borderwidth 20
    bltTest2 $graph legend -bd 20
    bltTest2 $graph legend -columns 2
    #bltTest2 $graph legend -exportselection
    #bltTest2 $graph legend -focusdashes
    #bltTest2 $graph legend -focusforeground
    bltTest2 $graph legend -font "times 18 bold italic"
    bltTest2 $graph legend -fg yellow
    bltTest2 $graph legend -foreground purple
    bltTest2 $graph legend -hide yes
    bltTest2 $graph legend -ipadx 20
    bltTest2 $graph legend -ipady 20
    #bltTest2 $graph legend -nofocusselectbackground
    #bltTest2 $graph legend -nofocusselectforeground
    bltTest2 $graph legend -padx 20
    bltTest2 $graph legend -pady 20
    bltTest2 $graph legend -position leftmargin
    bltTest2 $graph legend -position topmargin
    bltTest2 $graph legend -position bottommargin
    bltTest2 $graph legend -position "@250,100"
    bltTest2 $graph legend -raised yes
    bltTest2 $graph legend -relief groove
    bltTest2 $graph legend -rows 1
    #bltTest2 $graph legend -selectbackground
    #bltTest2 $graph legend -selectborderwidth
    #bltTest2 $graph legend -selectcommand
    #bltTest2 $graph legend -selectforeground
    #bltTest2 $graph legend -selectmode
    #bltTest2 $graph legend -selectrelief
    #bltTest2 $graph legend -takefocus
    bltTest2 $graph legend -title "Hello World"
    ##bltTest2 $graph legend -titleanchor center
    ##bltTest2 $graph legend -titlecolor red
    bltTest2 $graph legend -titlefont "times 24 bold italic"

    #$graph active
    #$graph bind
    #$graph curselection
    #$graph deactive
    #$graph focus
    #$graph get
    #$graph selection anchor
    #$graph selection clear
    #$graph selection clearall
    #$graph selection includes
    #$graph selection mark
    #$graph selection present
    #$graph selection set
    #$graph selection toggle
}

echo "done"
#bltPlotDestroy .line

