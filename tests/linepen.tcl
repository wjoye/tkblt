source base.tcl

set w .line
set graph [bltLineGraph $w]

$graph pen create foo -color red -showvalues y -symbol circle -dashes {4 4}
$graph element configure data2 -pen foo

echo "Testing Line Pen..."
$graph element configure data1

bltTest3 $graph pen foo -color yellow
bltTest3 $graph pen foo -dashes {8 3}
bltTest3 $graph pen foo -errorbarcolor green
bltTest3 $graph pen foo -errorbarwidth 2
bltTest3 $graph pen foo -errorbarcap 10
bltTest3 $graph pen foo -fill cyan
bltTest3 $graph pen foo -linewidth 3
bltTest3 $graph pen foo -offdash black
bltTest3 $graph pen foo -outline green
bltTest3 $graph pen foo -outlinewidth 5
bltTest3 $graph pen foo -pixels 20
bltTest3 $graph pen foo -showvalues none
bltTest3 $graph pen foo -symbol arrow
bltTest3 $graph pen foo -symbol cross
bltTest3 $graph pen foo -symbol diamond
bltTest3 $graph pen foo -symbol none
bltTest3 $graph pen foo -symbol plus
bltTest3 $graph pen foo -symbol scross
bltTest3 $graph pen foo -symbol splus
bltTest3 $graph pen foo -symbol square
bltTest3 $graph pen foo -symbol triangle
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

