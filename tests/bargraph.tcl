source base.tcl

set w .bar
set graph [bltBarGraph $w]

echo "Testing Bar Graph..."

# Graph
bltTest $graph -aspect 2
bltTest $graph -background red
bltTest $graph -barmode stacked
bltTest $graph -barmode aligned
bltTest $graph -barmode overlap
bltTest $graph -barwidth .15
#bltTest $graph -baseline
bltTest $graph -bd 50
bltTest $graph -bg green
bltTest $graph -bm 50
bltTest $graph -borderwidth 50
bltTest $graph -bottommargin 50
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
bltTest $graph -lm 50
bltTest $graph -plotbackground cyan
bltTest $graph -plotborderwidth 50
bltTest $graph -plotpadx 50
bltTest $graph -plotpady 50
bltTest $graph -plotrelief groove
bltTest $graph -relief groove
bltTest $graph -rightmargin 50
bltTest $graph -rm 50
#bltTest $graph -searchhalo
#bltTest $graph -searchmode
#bltTest $graph -searchalong
#bltTest $graph -stackaxes
#bltTest $graph -takefocus
bltTest $graph -title "This is a Title"
bltTest $graph -tm 50
bltTest $graph -topmargin 50
bltTest $graph -width 300
bltTest $graph -plotwidth 300
bltTest $graph -plotheight 300

##bltCmd $graph axis
bltCmd $graph cget -background
bltCmd $graph configure 
bltCmd $graph configure 
bltCmd $graph configure -background cyan
##bltCmd $graph crosshairs
##bltCmd $graph element
#bltCmd $graph extents
#bltCmd $graph inside
#bltCmd $graph invtransform
##bltCmd $graph legend
##bltCmd $graph marker
##bltCmd $graph pen
##bltCmd $graph postscript
#bltCmd $graph transform
##bltCmd $graph x2axis
##bltCmd $graph xaxis
##bltCmd $graph y2axis
##bltCmd $graph yaxis

echo "done"
bltPlotDestroy $w

