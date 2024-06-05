// optional file you can load into the q process to see what's going on during the test

.tst.nfo:{[M]
  -1 (string .z.Z),"  INFO: ",M
 }

.tst.err:{[M]
  -2 (string .z.Z)," ERROR: ",M
 }

.tst.zpw:{[U;P]
  .tst.nfo "Beginning '",(string U),"'"
 ;`.tst.fds upsert (.z.w;U)
 ;1b
 }

.tst.zpc:{[H]
  dct:exec from .tst.fds where fd = H
 ;$[not null dct`tst
   ;.tst.nfo "Ended ",string dct`tst
   ;.tst.nfo "Ended test"
   ]
 ;
 }

.u.upd:{[T;X]
  .tst.args,:enlist `T`X!(T;X)
 ;.tst.nfo "Received .u.upd message for table ",.Q.s1 T
 ;
 }

.tst.zps:{[M]
  value M
 ;.tst.args,:enlist (!/)enlist each (`.z.ps;M)
 ;(neg .z.w) M
 ;
 }

.tst.init:{
  .tst.fds:1!flip`fd`tst!"IS"$\:()
 ;.tst.args:enlist(::)
 ;.z.pw:.tst.zpw
 ;.z.pc:.tst.zpc
 ;.z.ps:.tst.zps
 ;system"p 30098"
 ;1b
 }

.tst.init[];
