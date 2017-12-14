/Mymalloc:/ {
    if(mallcnts[$NF] == 0) {
	mallocs[mi++] = $NF;
	mallcnts[$NF] = 1;
    } else mallcnts[$NF] = mallcnts[$NF] + 1;
}

/Myfree:/ {
    if(freecnts[$NF] == 0) {
	free[fi++] = $NF;
	freecnts[$NF] = 1;
    } else freecnts[$NF] = freecnts[$NF] + 1;
}

END {
    print "Malloc counts:", mi, "Free counts:", fi;
    for(i = 0; i < mi; i++) {
	print "Malloc:", mallocs[i], "count", mallcnts[mallocs[i]];
    }
    for(i = 0; i < fi; i++) {
	print "Free:", free[i], "count", freecnts[free[i]];
    }
}
