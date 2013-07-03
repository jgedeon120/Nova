UPDATE_FILE=novaInstallHelper.sh.gpg
UPDATE_SCRIPT=novaInstallHelper.sh

rm -f $UPDATE_FILE
rm -f $UPDATE_SCRIPT

echo "############################ Stopping Novad and Haystack ############################"
novacli stop nova
novacli stop haystack

echo "############################ Downloading Update File ############################"
wget http://pherricoxide.dyndns.org:42080/$UPDATE_FILE


echo "############################ Checking Update File Integrity ############################"
gpg --batch --no-default-keyring --keyring "/usr/share/nova/sharedFiles/datasoftkeys.gpg" --verify $UPDATE_FILE
if [ $? -ne 0 ]
then
	echo "ERROR: signature of update file can not be verified. Cancelling.\n"
	exit 1
fi

echo "############################ Decrypting Update File ############################"
gpg --batch --no-default-keyring --keyring "/usr/share/nova/sharedFiles/datasoftkeys.gpg" --output $UPDATE_SCRIPT --decrypt $UPDATE_FILE
if [ $? -ne 0 ]
then
	echo "ERROR: update file can not be decrypted. Cancelling.\n"
	exit 1
fi


/bin/bash $UPDATE_SCRIPT

