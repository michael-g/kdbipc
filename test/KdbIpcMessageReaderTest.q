
.unz.write10kQa:{[D]
  (` sv D,`tenKQa.zipc) 1: -18!10000#.Q.a
 }
.unz.debug10kQa:{
   ipc:-18!10000#.Q.a
  ;(sums 0 4 4 4 1 8 1 8 1 8 1 8 1 8 1 9 1 11 1 11 1 12 1 10 1 11 1 11 1 11 1 10 1 11 1 11 1 11 1 11 1)cut ipc
 }

.unz.writeZipcFiles:{[D]
  if[not 11h~type key D
    ;'"Expected D to be a directory"
    ]
 ;.unz.write10kQa D
 }