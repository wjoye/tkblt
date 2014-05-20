source base.tcl

set w .bar
set graph [bltBarGraph $w]

$graph element configure data1 -showvalues y

$graph pen create foo -showvalues y -color purple
$graph element activate data3

echo "Bar Element"

bltTest3 $graph element data3 -activepen foo
bltTest3 $graph element data2 -background yellow
bltTest3 $graph element data2 -barwidth 10
bltTest3 $graph element data2 -bd 4
bltTest3 $graph element data2 -bg yellow
#bltTest3 $graph element data2 -bindtags
bltTest3 $graph element data2 -borderwidth 4
bltTest3 $graph element data2 -color yellow
bltTest3 $graph element data1 -data {0.2 8 0.4 20 0.6 31 0.8 41 1.0 50 1.2 59 1.4 65 1.6 70 1.8 75 2.0 85}
bltTest3 $graph element data2 -errorbarcolor green
bltTest3 $graph element data2 -errorbarwidth 2
bltTest3 $graph element data2 -errorbarcap 10
bltTest3 $graph element data2 -fg yellow
bltTest3 $graph element data1 -fill cyan
bltTest3 $graph element data2 -foreground green
bltTest3 $graph element data2 -hide yes
bltTest3 $graph element data2 -label "This is a test"
bltTest3 $graph element data2 -legendrelief groove
bltTest3 $graph element data2 -mapx x2
bltTest3 $graph element data2 -mapy y2
bltTest3 $graph element data1 -outline red
bltTest3 $graph element data2 -pen foo
bltTest3 $graph element data2 -relief flat
bltTest3 $graph element data2 -showerrorbars no
bltTest3 $graph element data1 -showvalues none
bltTest3 $graph element data1 -showvalues x
bltTest3 $graph element data1 -showvalues both
#bltTest3 $graph element data2 -stack
#bltTest3 $graph element data2 -styles
bltTest3 $graph element data1 -valueanchor n
bltTest3 $graph element data1 -valuecolor cyan
bltTest3 $graph element data1 -valuefont "times 18 bold italic"
bltTest3 $graph element data1 -valueformat "%e"
bltTest3 $graph element data1 -valuerotate 45
#bltTest3 $graph element data2 -weights
bltTest3 $graph element data1 -x {0 .2 .4 .6 .8 1 1.2 1.4 1.6 1.8}
bltTest3 $graph element data1 -xdata {0 .2 .4 .6 .8 1 1.2 1.4 1.6 1.8}
bltTest3 $graph element data2 -xerror {.1 .1 .1 .1 .1 .1 .1 .1 .1 .1 .1}
#bltTest3 $graph element data2 -xhigh
#bltTest3 $graph element data2 -xlow
bltTest3 $graph element data1 -y {8 20 31 41 50 59 65 70 75 85} 
bltTest3 $graph element data1 -ydata {8 20 31 41 50 59 65 70 75 85}
bltTest3 $graph element data2 -yerror {5 5 5 5 5 5 5 5 5 5 5}
#bltTest3 $graph element data2 -yhigh
#bltTest3 $graph element data2 -ylow

bltCmd $graph element activate data2
bltCmd $graph element deactivate data2
#bltCmd $graph element bind data1 <Button-1> [list puts "%x %y"]
bltCmd $graph element cget data1 -smooth
bltCmd $graph element configure data1
bltCmd $graph element configure data1 -smooth
#bltCmd $graph element closest 50 50
#bltCmd $graph element closest 50 50 data1 data2
bltCmd $graph element create data4
bltCmd $graph element create data5
bltCmd $graph element delete data4 data5
bltCmd $graph element exists data1
#bltCmd $graph element get
bltCmd $graph element lower data1
bltCmd $graph element lower data2 data3
bltCmd $graph element names
bltCmd $graph element names data1
bltCmd $graph element raise data2
bltCmd $graph element raise data2 data3
bltCmd $graph element raise data1
bltCmd $graph element show data2
bltCmd $graph element show {data1 data2 data3}
bltCmd $graph element type data1

echo "done"
bltPlotDestroy $w

