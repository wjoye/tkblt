source base.tcl

set w .line
set graph [bltLineGraph $w]
#set graph [bltBarGraph $w]

$graph postscript output foo.ps

#echo "done"
#bltPlotDestroy $w

