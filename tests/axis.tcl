source base.tcl

set w .line
set graph [bltLineGraph $w]

$graph xaxis configure -title "X Axis" -limitsformat "%g"

echo "Testing Axis..."

#bltCmd $graph axis activate
#bltCmd $graph axis bind
#bltCmd $graph axis cget
#bltCmd $graph axis configure
#bltCmd $graph axis create
#bltCmd $graph axis deactivate
#bltCmd $graph axis delete
#bltCmd $graph axis focus
#bltCmd $graph axis get
#bltCmd $graph axis invtransform
#bltCmd $graph axis limits
#bltCmd $graph axis margin
#bltCmd $graph axis names
#bltCmd $graph axis transform
#bltCmd $graph axis type
#bltCmd $graph axis view

echo "done"
bltPlotDestroy $w

