for f in test/*.rkr 
do
  rock $f  || echo FAILED
done
