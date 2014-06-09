source base.tcl

set w .line
set graph [bltLineGraph $w]

$graph axis configure x -title "X\nAxis" -limitsformat "%g"
$graph axis configure y -title "Y\nAxis"

$graph element configure data1 -dash {8 3} -showvalues y -smooth step -symbol circle -outline yellow -outlinewidth 3 -pixels 10 -valuefont "times 14 italic" -valuerotate 45

$graph postscript output foo.ps

#set graph [bltBarGraph $w]

#echo "done"
#bltPlotDestroy $w

