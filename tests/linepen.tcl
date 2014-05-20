source base.tcl

set w .line
set graph [bltLineGraph $w]

$graph pen create foo -showvalues y -symbol circle -dashes "8 4"
$graph element configure data2 -pen foo

echo "Testing Line Pen..."

bltTest3 $graph pen foo -color red
bltTest3 $graph pen foo -dashes "5 8 3"
bltTest3 $graph pen foo -errorbarcolor yellow
bltTest3 $graph pen foo -errorbarwidth 2
bltTest3 $graph pen foo -fill green
bltTest3 $graph pen foo -linewidth 2
bltTest3 $graph pen foo -offdash yellow
bltTest3 $graph pen foo -outline green
bltTest3 $graph pen foo -outlinewidth 2
bltTest3 $graph pen foo -pixels 20
bltTest3 $graph pen foo -showvalues none
bltTest3 $graph pen foo -symbol square
bltTest3 $graph pen foo -valueanchor n
bltTest3 $graph pen foo -valuecolor cyan
bltTest3 $graph pen foo -valuefont "times 18 bold italic"
bltTest3 $graph pen foo -valueformat "%e"
bltTest3 $graph pen foo -valuerotate 45

bltCmd $graph pen cget foo -color
bltCmd $graph pen configure foo
bltCmd $graph pen configure foo -color
bltCmd $graph pen create bar
bltCmd $graph pen delete bar
bltCmd $graph pen names 
bltCmd $graph pen type foo

echo "done"
bltPlotDestroy $w

