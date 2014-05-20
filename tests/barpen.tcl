source base.tcl

set w .bar
set graph [bltBarGraph $w]

$graph pen create foo -color red -showvalues y
$graph element configure data2 -pen foo

echo "Testing Bar Pen..."

bltTest3 $graph pen foo -background yellow
bltTest3 $graph pen foo -bd 4
bltTest3 $graph pen foo -bg yellow
bltTest3 $graph pen foo -borderwidth 4
bltTest3 $graph pen foo -color yellow
bltTest3 $graph pen foo -errorbarcolor green
bltTest3 $graph pen foo -errorbarwidth 2
bltTest3 $graph pen foo -errorbarcap 10
bltTest3 $graph pen foo -fg yellow
bltTest3 $graph pen foo -fill cyan
bltTest3 $graph pen foo -foreground green
bltTest3 $graph pen foo -outline red
bltTest3 $graph pen foo -relief flat
bltTest3 $graph pen foo -showerrorbars no
bltTest3 $graph pen foo -showvalues none
bltTest3 $graph pen foo -showvalues x
bltTest3 $graph pen foo -showvalues both
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

