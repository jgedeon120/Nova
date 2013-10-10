#!/bin/bash

PWD=`pwd`

SAVE=$PWD'/'

NEW_PERM=$SUDO_USER:$SUDO_USER

BUILDDIR=~/nova-update

echo "Build Dir is $BUILDDIR"

check_err() {
	ERR=$?
	if [ "$ERR" -ne "0" ] ; then
		echo "Error occurred during build process; terminating script!"
		exit $ERR
	fi
}

rm -fr ${BUILDDIR}
mkdir -p ${BUILDDIR}
cd ${BUILDDIR}


echo "##############################################################################"
echo "#                          Downloading updates... please wait.               #"
echo "##############################################################################"

wget --read-timeout 10 -nv http://www.projectnova.org/updates/Honeyd.tar
check_err
tar -xf Honeyd.tar
check_err

wget --read-timeout 10 -nv http://www.projectnova.org/updates/Nova.tar
check_err
tar -xf Nova.tar
check_err

echo "##############################################################################"
echo "#                          NOVA DEPENDENCY CHECK                             #"
echo "##############################################################################"
apt-get update
apt-get -y install git build-essential libann-dev libpcap0.8-dev libboost-program-options-dev libboost-serialization-dev sqlite3 libsqlite3-dev libcurl3 libcurl4-gnutls-dev iptables libevent-dev libprotoc-dev protobuf-compiler libdumbnet-dev libpcap-dev libpcre3-dev libedit-dev bison flex libtool automake libcap2-bin libboost-system-dev libboost-filesystem-dev python perl tcl liblinux-inotify2-perl libfile-readbackwards-perl
check_err

echo "##############################################################################"
echo "#                          Backing up current configuration files...         #"
echo "##############################################################################"

rm -fr ~/.config/nova-oldConfigs
mv ~/.config/nova ~/.config/nova-oldConfigs

rm -fr ~/.config/honeyd-oldConfigs
mv ~/.config/honeyd ~/.config/honeyd-oldConfigs

echo "##############################################################################"
echo "#                              BUILDING HONEYD                               #"
echo "##############################################################################"
cd ${BUILDDIR}/honeyd
./autogen.sh
check_err
automake
check_err
./configure
check_err
make -j2 -s
check_err
make install
check_err

cd ${BUILDDIR}/Nova
check_err

echo "##############################################################################"
echo "#                             BUILDING NOVA                                  #"
echo "##############################################################################"
cd ${BUILDDIR}/Nova/Quasar
bash getDependencies.sh
check_err
chown -R -f $NEW_PERM node-v0.8.5/
chown -f $NEW_PERM node-v0.8.5.tar.gz
cd ${HOME}
chown -R $NEW_PERM .npm/
check_err
cd ${BUILDDIR}/Nova/Quasar
npm install -g forever
check_err

cd ${BUILDDIR}/Nova
make -j2 -s debug
check_err
make uninstall-files
make install
check_err

bash ${BUILDDIR}/Nova/Installer/nova_init

cd $HOME
chown -R -f $NEW_PERM .node-gyp/

cd /usr/share/honeyd/scripts/
chown -R -f $NEW_PERM misc/

echo "##############################################################################"
echo "#                       Restoring old configuration files                    #"
echo "##############################################################################"

rm -fr ~/.config/nova
cp -fr ~/.config/nova-oldConfigs ~/.config/nova

rm -fr ~/.config/honeyd
cp -fr ~/.config/honeyd-oldConfigs ~/.config/honeyd


echo "##############################################################################"
echo "#                       Copying over any new configuration files             #"
echo "##############################################################################"
yes n | cp -R -i ${BUILDDIR}/Nova/Installer/userFiles/* ~/.config/nova/

echo "##############################################################################"
echo "#                       Updating version.txt file                            #"
echo "##############################################################################"

cp -f ${BUILDDIR}/Nova/Installer/userFiles/config/version.txt ~/.config/nova/config/version.txt

echo "##############################################################################"
echo "#                       Fixing file permissions                              #"
echo "##############################################################################"
chown -R $NEW_PERM ~/.config/nova
chown -R $NEW_PERM ~/.config/honeyd

# Needs to be accessed by the nobody user
chmod a+rw ~/.config/nova/data/scriptAlerts.db*

echo "##############################################################################"
echo "#                                    DONE                                    #"
echo "##############################################################################"
