#!/bin/bash
length=$(($#-1))
OPTIONS=${@:1:$length}
REPONAME="${!#}"
CWD=$PWD
#echo -e "\n\nInstalling commons libraries...\n\n"
#COMMONS="so-commons-library"
#git clone "https://github.com/sisoputnfrba/${COMMONS}.git" $COMMONS
#cd $COMMONS
#sudo make uninstall
#make all
#sudo make install
#cd $CWD
echo -e "\n\nBuilding projects...\n\n"
mkdir -p ./shared/obj
cd $PWD
make -C ./Discordiador
make -C ./Discordiador Discordiador
make -C ./i-MongoStore
make -C ./i-MongoStore i-MongoStore
make -C ./Mi-RAM\ HQ
make -C ./Mi-RAM\ HQ Mi-RAM-HQ

echo -e "\n\nDeploy done!\n\n"