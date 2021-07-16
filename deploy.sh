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

# Biblioteca para dibujar el mapa (revisar)
git clone https://github.com/sisoputnfrba/so-nivel-gui-library/
cd so-nivel-gui-library
sudo make uninstall
make all
sudo make install
cd $CWD

# Repositorio con scripts para los tests
cd Discordiador
git clone https://github.com/sisoputnfrba/a-mongos-pruebas.git
cd $CWD


echo -e "\n\nBuilding projects...\n\n"
cd $PWD
make -C ./Discordiador
make -C ./i-MongoStore
make -C ./Mi-RAM\ HQ

echo -e "\n\nDeploy done!\n\n"