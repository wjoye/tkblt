source base.tcl

set w .line
set graph [bltLineGraph $w]

$graph axis configure x -bd 2 -background cyan -title "X Axis" -limitsformat "%g"
$graph axis configure y -bd 2 -background cyan
bltCmd $graph axis activate y

echo "Testing Axis..."
bltCmd $graph axis configure x

bltTest3 $graph axis y -activeforeground red
bltTest3 $graph axis y -activerelief sunken
#bltTest3 $graph axis x -autorange 10
bltTest3 $graph axis x -background yellow
bltTest3 $graph axis x -bg blue
#bltTest3 $graph axis x -bindtags
bltTest3 $graph axis y -bd 4
bltTest3 $graph axis y -borderwidth 4
#bltTest3 $graph axis x -checklimits
bltTest3 $graph axis x -color red
#bltTest3 $graph axis x -command
bltTest3 $graph axis x -descending yes
bltTest3 $graph axis x -exterior no
bltTest3 $graph axis x -fg magenta
bltTest3 $graph axis x -foreground yellow
bltTest3 $graph axis x -grid no
bltTest3 $graph axis x -gridcolor blue
bltTest3 $graph axis x -griddashes {8 3}
bltTest3 $graph axis x -gridlinewidth 2
bltTest3 $graph axis x -gridminor no
bltTest3 $graph axis x -gridminorcolor blue
bltTest3 $graph axis x -gridminordashes {8 3}
bltTest3 $graph axis x -gridminorlinewidth 2
bltTest3 $graph axis x -hide yes
##bltTest3 $graph axis x -justify left
bltTest3 $graph axis x -labeloffset yes
bltTest3 $graph axis x -limitscolor red
bltTest3 $graph axis x -limitsfont "times 18 bold italic"
bltTest3 $graph axis x -limitsformat "%e"
bltTest3 $graph axis x -linewidth 2
bltTest3 $graph axis x -logscale yes
#bltTest3 $graph axis x -loosemin
#bltTest3 $graph axis x -loosemax
#bltTest3 $graph axis x -majorticks
#bltTest3 $graph axis x -max
#bltTest3 $graph axis x -min
#bltTest3 $graph axis x -minorticks
bltTest3 $graph axis x -relief groove
bltTest3 $graph axis x -rotate 45
#bltTest3 $graph axis x -scrollcommand
#bltTest3 $graph axis x -scrollincrement
#bltTest3 $graph axis x -scrollmax
#bltTest3 $graph axis x -scrollmin
##bltTest3 $graph axis x -shiftby 10
bltTest3 $graph axis x -showticks no
bltTest3 $graph axis x -stepsize 10
bltTest3 $graph axis x -subdivisions 4
##bltTest3 $graph axis x -tickanchor n
bltTest3 $graph axis x -tickfont {times 12 bold italic}
bltTest3 $graph axis x -ticklength 20
bltTest3 $graph axis x -tickdefault 10
bltTest3 $graph axis x -title {This is a Title}
bltTest3 $graph axis x -titlealternate yes
bltTest3 $graph axis x -titlecolor yellow
bltTest3 $graph axis x -titlefont {times 24 bold italic}

#bltCmd $graph axis activate foo
#bltCmd $graph axis bind x
bltCmd $graph axis cget x -color
bltCmd $graph axis configure x
bltCmd $graph axis configure x -color
#bltCmd $graph axis create foo
#bltCmd $graph axis deactivate foo
#bltCmd $graph axis delete foo
#bltCmd $graph axis focus x
#bltCmd $graph axis get x
#bltCmd $graph axis invtransform x
#bltCmd $graph axis limits x
#bltCmd $graph axis margin x
#bltCmd $graph axis names x
#bltCmd $graph axis transform x
#bltCmd $graph axis type x
#bltCmd $graph axis view x

#bltCmd $graph xaxis activate
#bltCmd $graph xaxis bind
bltCmd $graph xaxis cget -color
bltCmd $graph xaxis configure
bltCmd $graph xaxis configure -color
#bltCmd $graph xaxis deactivate
#bltCmd $graph xaxis invtransform
#bltCmd $graph xaxis limits
#bltCmd $graph xaxis transform
#bltCmd $graph xaxis use
#bltCmd $graph xaxis view

echo "done"
bltPlotDestroy $w

