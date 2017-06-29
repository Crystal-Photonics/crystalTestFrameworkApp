"C:\Program Files\Git\bin\git" log -1 --format="format:#define GITHASH 0x%h"  > vc.h
ECHO " " >> vc.h
"C:\Program Files\Git\bin\git" log -1 --date=short --format="format:#define GITDATE \"%ad\"" >> vc.h
ECHO " " >> vc.h
"C:\Program Files\Git\bin\git" log -1 --date=short --format="format:#define GITUNIX %ct" >> vc.h
