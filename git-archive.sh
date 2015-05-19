#!/bin/bash
DT=`date +%Y%m%d`
git archive --remote=git@172.16.99.55:git/wks_oic_raclient.git master --format=tar --prefix=wks_oic_raclient/ | gzip > wks_oic_raclient.$DT.tgz

