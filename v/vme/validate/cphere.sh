#!/bin/bash
echo $0
if [ $0 != "./cphere.sh" ] ;then
  cat - <<-EOF
Start: 
./cphere mkdir            -prepare ~/validate only
./cphere                  -above + build
EOF
  exit
fi
mkdir -p ~/validate ; cp MANIFEST setup.py ~/validate/ ; cd ~/validate
p1="$VMEBDIR/myw.py $VMEBDIR/trigdb.py $VMEBDIR/txtproc.py $VMEBDIR/downscaling.py"
p2="$VMECFDIR/TRG_DBED/parted.py $VMECFDIR/TRG_DBED/validate.py \
$VMECFDIR/TRG_DBED/syncdg.py $VMECFDIR/TRG_DBED/preproc.py $VMECFDIR/TRG_DBED/trgglobs.py"
pyfiles="myw.py trigdb.py txtproc.py parted.py syncdg.py validate.py downscaling.py preproc.py trgglobs.py"
# aliases.exe: not added to pyfiles (it is needed if tkinter/x86_64 arch .rpm is created)
#echo $pyfiles
cp $p1 ./
cp $p2 ./
cp $VMECFDIR/pydim/aliasesmain.c ./
#g++ -g aliasesmain.c -o aliases.exe
g++ aliasesmain.c -o aliases.exe
#chmod a-x aliases.exe
#echo "aliasesexe" >alases
if [ "$1" = "mkdir" ] ;then
  echo "~/validate prepared"
  exit
fi
#echo 'Copy of these files made to local directory:'
#ls -l *.py
#echo 'python setup.py sdist   (bdist_rpm creates .rpm distribution)'
#see readme for explanation of next line:
#echo "%_unpackaged_files_terminate_build 0" >> ~/.rpmmacros
echo "%_binaries_in_noarch_packages_terminate_build 0" > ~/.rpmmacros
echo "%_unpackaged_files_terminate_build 0" >> ~/.rpmmacros
#read -p "pwd:`pwd` continue? (y/n):" yn
#export DISTUTILS_DEBUG="ALASES"
python setup.py bdist_rpm  >/tmp/v.log 2>&1
echo "pwd:`pwd` Do:"
#echo 'scp dist/validate-1.0.tar.gz $aj11:'
#echo 'scp ~/validate/dist/validate-2.3-1.noarch.rpm $lxp:t/'
# 2.5: strict validation added (pydim needed then)!
echo
#echo '(no tkinter dependance) scp ~/validate/dist/validate-5.1-1.noarch.rpm $lxp:t/'
#echo '(effectively filtered out with *) scp ~/validate/dist/validate-5.3-1.noarch.rpm $lxp:t/'
echo 'scp ~/validate/dist/validate-5.11-1.noarch.rpm $lxp:t/'
read -p "remove $pyfiles in pwd:`pwd` ? (y/n):" yn
if [ "$yn" = 'y' ] ;then
  rm $pyfiles
fi

