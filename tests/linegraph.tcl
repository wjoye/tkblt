source base.tcl

set w .line
set graph [bltLineGraph $w]

echo "Testing LineGraph..."

bltTest $graph -aspect 2
bltTest $graph -background red
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
bltTest $graph -fg blue
bltTest $graph -font {times 36 bold italic}
bltTest $graph -foreground cyan
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
#bltTest $graph -searchhalo
#bltTest $graph -searchmode
#bltTest $graph -searchalong
#bltTest $graph -stackaxes
#bltTest $graph -takefocus
bltTest $graph -title "This is a Title"
bltTest $graph -tm 50
bltTest $graph -topmargin 50
#bltTest $graph -topvariable
bltTest $graph -width 300
bltTest $graph -plotwidth 300
bltTest $graph -plotheight 300

echo "done"
bltPlotDestroy $w

