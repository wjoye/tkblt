source base.tcl

set w .line
set graph [bltLineGraph $w]

$graph xaxis configure -title "X Axis"

#bltTest2 $graph xaxis -activeforeground red
#bltTest2 $graph xaxis -activerelief grooved
#bltTest2 $graph xaxis -autorange 10
bltTest2 $graph xaxis -background yellow
bltTest2 $graph xaxis -bg cyan
#bltTest2 $graph xaxis -bindtags
##bltTest2 $graph xaxis -bd 2
##bltTest2 $graph xaxis -borderwidth 4
#bltTest2 $graph xaxis -checklimits
bltTest2 $graph xaxis -color red
#bltTest2 $graph xaxis -command
bltTest2 $graph xaxis -descending yes
bltTest2 $graph xaxis -exterior no
bltTest2 $graph xaxis -fg magenta
bltTest2 $graph xaxis -foreground yellow
bltTest2 $graph xaxis -grid no
bltTest2 $graph xaxis -gridcolor green
bltTest2 $graph xaxis -griddashes dashdot
bltTest2 $graph xaxis -gridlinewidth 2
bltTest2 $graph xaxis -gridminor no
bltTest2 $graph xaxis -gridminorcolor blue
bltTest2 $graph xaxis -gridminordashes dashdot
bltTest2 $graph xaxis -gridminorlinewidth 2
bltTest2 $graph xaxis -hide yes
##bltTest2 $graph xaxis -justify left
bltTest2 $graph xaxis -labeloffset yes
#bltTest2 $graph xaxis -limitscolor
#bltTest2 $graph xaxis -limitsfont
#bltTest2 $graph xaxis -limitsformat
bltTest2 $graph xaxis -linewidth 2
bltTest2 $graph xaxis -logscale yes
#bltTest2 $graph xaxis -loose
#bltTest2 $graph xaxis -majorticks
#bltTest2 $graph xaxis -max
#bltTest2 $graph xaxis -min
#bltTest2 $graph xaxis -minorticks
##bltTest2 $graph xaxis -relief groove
bltTest2 $graph xaxis -rotate 45
#bltTest2 $graph xaxis -scrollcommand
#bltTest2 $graph xaxis -scrollincrement
#bltTest2 $graph xaxis -scrollmax
#bltTest2 $graph xaxis -scrollmin
#bltTest2 $graph xaxis -shiftby
bltTest2 $graph xaxis -showticks no
bltTest2 $graph xaxis -stepsize 10
bltTest2 $graph xaxis -subdivisions 4
##bltTest2 $graph xaxis -tickanchor n
bltTest2 $graph xaxis -tickfont {times 12 bold italic}
bltTest2 $graph xaxis -ticklength 20
bltTest2 $graph xaxis -tickdefault 10
bltTest2 $graph xaxis -title {This is a Title}
#bltTest2 $graph xaxis -titlealternate yes
bltTest2 $graph xaxis -titlecolor yellow
bltTest2 $graph xaxis -titlefont {times 24 bold italic}
#bltTest2 $graph xaxis -use

#bltCmd $graph xaxis activate
#bltCmd $graph xaxis bind
#bltCmd $graph xaxis cget
#bltCmd $graph xaxis configure
#bltCmd $graph xaxis deactivate
#bltCmd $graph xaxis invtransform
#bltCmd $graph xaxis limits
#bltCmd $graph xaxis transform
#bltCmd $graph xaxis use
#bltCmd $graph xaxis view

#bltCmd $graph vaxis activate
#bltCmd $graph vaxis bind
#bltCmd $graph vaxis cget
#bltCmd $graph vaxis configure
#bltCmd $graph vaxis create
#bltCmd $graph vaxis deactivate
#bltCmd $graph vaxis delete
#bltCmd $graph vaxis focus
#bltCmd $graph vaxis get
#bltCmd $graph vaxis invtransform
#bltCmd $graph vaxis limits
#bltCmd $graph vaxis margin
#bltCmd $graph vaxis names
#bltCmd $graph vaxis transform
#bltCmd $graph vaxis type
#bltCmd $graph vaxis view

echo "done"
bltPlotDestroy $w

