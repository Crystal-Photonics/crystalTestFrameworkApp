git log -1 --format="format:#define GITHASH 0x%h"  > vc.h
echo " " >> vc.h
git log -1 --date=short --format="format:#define GITDATE \"%ad\"" >> vc.h
echo " " >> vc.h
git log -1 --date=short --format="format:#define GITUNIX %ct" >> vc.h
